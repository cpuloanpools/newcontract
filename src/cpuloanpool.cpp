#include <cpuloanpool.hpp>

static inline time_point_sec current_time_point_sec()
{
   return time_point_sec(current_time_point());
}

ACTION cpuloanpool::setconfig(
   const uint64_t unstakeSeconds,
   const float fee,
   const asset min_amount,
   const uint64_t min_days, 
   const uint64_t max_days, 
   const uint64_t renew_dis, 
   const uint64_t renew_mult)
{
   require_auth(get_self());
   auto config_ = config.begin();
   check(min_amount.symbol == WAX_S, "Incorrect symbol");
   if (config_ == config.end())
   {
      config.emplace(get_self(), [&](auto &v)
                     {
         v.id = config.available_primary_key();
         v.unstakeSeconds = unstakeSeconds;
         v.min_amount = asset(min_amount.amount,WAX_S);
         v.fee_rate = fee;
         v.min_days = min_days;
         v.max_days = max_days;
         v.renew_dis = renew_dis;
         v.renew_mult = renew_mult;
         v.earnings = asset(0,WAX_S);
         v.index = 1; });
   }
   else
      config.modify(config_, get_self(), [&](auto &v)
      {
         v.unstakeSeconds = unstakeSeconds;
         v.min_amount = asset(min_amount.amount,WAX_S);
         v.min_days = min_days;
         v.max_days = max_days;
         v.fee_rate = fee;
         v.renew_dis = renew_dis;
         v.renew_mult = renew_mult; });
}

ACTION cpuloanpool::setindex()
{
   require_auth(get_self());
   check(config.begin() != config.end(), "No Config Found");
   auto itr = pool_t.begin();
   check(itr != pool_t.end(),"Pool Not Found");
   uint64_t index = config.begin()->index;
   index = index >= itr->pools.size() ? 1 : index + 1;
   config.modify(config.begin(), get_self(), [&](auto &v)
   { v.index = index; });
}

ACTION cpuloanpool::regpool(const name pool)
{
   require_auth(get_self());
   auto itr = pool_t.begin();
   pair_name t_pair;
   vector<pair_name> temp;
   if (itr == pool_t.end())
   {
      t_pair.pool = pool;
      t_pair.stakedusers = 0;
      temp.push_back(t_pair);
      pool_t.emplace(get_self(), [&](auto &v)
                     {
         v.id = pool_t.available_primary_key();
         v.pools = temp; });
   }
   else
   {
      temp = itr->pools;
      int p = pfinder(itr->pools, pool);
      check(p == -1, "Pool Already Exists");
      t_pair.pool = pool;
      t_pair.stakedusers = 0;
      temp.push_back(t_pair);
      pool_t.modify(itr, get_self(), [&](auto &v)
      { v.pools = temp; });
   }
}

ACTION cpuloanpool::unregpool(const name pool)
{
   require_auth(get_self());
   auto itr = pool_t.begin();
   check(itr != pool_t.end(), "Pool Table not found");
   int p = pfinder(itr->pools, pool);
   check(p != -1, "Pool Don't Exists");
   vector<pair_name> temp = itr->pools;
   temp.erase(temp.begin() + p);
   pool_t.modify(itr, get_self(), [&](auto &v)
   { v.pools = temp; });
}

void cpuloanpool::feepayment(const name &from, const name &to, const asset &quantity, const string &memo)
{
   if (from == get_self() || from == name("eosio.stake"))
      return;

   if (memo != "")
   {
         auto itm = config.begin();
         check(itm != config.end(), "No Config Found");
         check(quantity.amount > 0, "Only Positive Amount Allowed");
         check(quantity.amount >= itm->min_amount.amount, "Must be greater than min. fees");
         vector<string> temp = getVector('%', memo);
         
         string type = temp[0];
         uint64_t days = 1;
         name receiver_account;
         asset stake_cpu = asset(0,WAX_S);
         uint64_t unstakeTime = 0;
         if(type == "add") {
            check(temp.size() == 2,"Invalid Memo");
            receiver_account = name(temp[1]);
            auto fee_temp = getAddAmount(receiver_account,quantity.amount);
            stake_cpu.amount = fee_temp;
         }
         else{
            check(temp.size() == 3,"Invalid Memo");
            days = std::stoul(temp[1]);
            receiver_account = name(temp[2]);
            check(days >= itm->min_days, "Must be greater than min. days");
            check(days <= itm->max_days, "Must be less than max. days");
            if(type == "renew") {
               check(days % itm->renew_mult == 0,"Invalid Day Input");
               auto fee_temp = getRenew_Fees(receiver_account,days);
               check(quantity.amount >= fee_temp,"Invalid Fees");
               stake_cpu.amount = fee_temp;
            }
            else{
               unstakeTime = current_time_point_sec().utc_seconds + (itm->unstakeSeconds * days);
               stake_cpu.amount = getAmount(quantity.amount,days);
            }
         }
         check(is_account(receiver_account), "Invalid Account");
         stakecpu(from, receiver_account, type, stake_cpu, unstakeTime, days);
         config.modify(itm, get_self(), [&](auto &v)
         { v.earnings += quantity; });
   }
}

float cpuloanpool::getAmount(uint64_t fee, uint64_t days){
   auto config_ = config.begin();
   check(config_ != config.end(),"Config not found");
   float temp = config_->fee_rate / 100;
   float fee_per_day = fee / days;
   uint64_t stake_amount = (fee_per_day / temp);
   return stake_amount;
}

float cpuloanpool::getRenew_Fees(name account, uint64_t days){
   auto config_ = config.begin();
   check(config_ != config.end(),"Config not found");
   auto stakes = staking.require_find(account.value,"User Not Found");
   uint64_t staked_cpu = stakes->stakedcpu.amount;
   float temp = config_->fee_rate / 100;
   float standard_fees = staked_cpu * temp * days;
   float discounted =  (double(config_->renew_dis) / 100) * standard_fees;
   return uint64_t(standard_fees - discounted);
}

float cpuloanpool::getAddAmount(name account,uint64_t fee){
   auto config_ = config.begin();
   check(config_ != config.end(),"Config not found");
   auto stakes = staking.require_find(account.value,"User Not Found");
   uint64_t unstake = stakes->unstakeTime;
   check(current_time_point_sec().utc_seconds < unstake,"Staking Already Expired for this account");
   uint64_t seconds_to_unstake = unstake - current_time_point_sec().utc_seconds;
   if(seconds_to_unstake < (3600*12)) seconds_to_unstake = 3600*12;
   double fee_per_time = (config_->fee_rate / config_->unstakeSeconds) * seconds_to_unstake;
   float fee_temp = fee_per_time / 100;
   uint64_t stake_amount = fee / fee_temp;
   return stake_amount;
}

ACTION cpuloanpool::checkrefund(const name pool)
{
   require_auth(get_self());
   auto itp = pool_t.begin();
   int p = pfinder(itp->pools, pool);
   check(p != -1, "Invalid Pool");
   auto by_pool_name_index = staking.get_index<name("account")>();
   auto lower_itr = by_pool_name_index.lower_bound(pool.value);
   auto upper_itr = by_pool_name_index.upper_bound(pool.value);
   check(lower_itr != by_pool_name_index.end(), "No Staking found for this pool");
   while (lower_itr != upper_itr && lower_itr != by_pool_name_index.end())
   {
      if (current_time_point_sec().utc_seconds >= lower_itr->unstakeTime)
      {
         inlineunstak(lower_itr->account);
         lower_itr = by_pool_name_index.lower_bound(pool.value);
         upper_itr = by_pool_name_index.upper_bound(pool.value);
      }
      else
         lower_itr++;
   }
}

ACTION cpuloanpool::reset(const string table)
{
   require_auth(get_self());
   int i = 0;
   if (table == "config")
   {
      auto config1 = config.begin();
      while (config1 != config.end())
      {
         config1 = config.erase(config1);
      }
   }
   else if (table == "staking")
   {
      auto stake = staking.begin();
      while (stake != staking.end())
      {
         stake = staking.erase(stake);
         i++;
         if (i >= 500)
            break;
      }
   }
   else if (table == "pools")
   {
      auto itr = pool_t.begin();
      while (itr != pool_t.end())
      {
         itr = pool_t.erase(itr);
      }
   }
}

ACTION cpuloanpool::adminstake(const name account, const string type, const asset stake_cpu_asset, const uint64_t days)
{
   require_auth(get_self());
   check(type == "add" || type == "renew" || type == "new","Invalid Type");
   if(type == "add") check(days == 0,"For More WAX days should be 0");
   stakecpu(get_self(), account, type, stake_cpu_asset, (current_time_point_sec().utc_seconds + (config.begin()->unstakeSeconds * days)), days);
}

ACTION cpuloanpool::adminunstake(const name account)
{
   require_auth(get_self());
   inlineunstak(account);
}

ACTION cpuloanpool::waxrefund(const name pool)
{
   require_auth(get_self());
   auto itr = pool_t.begin();
   check(itr != pool_t.end(), "Pool Table Not found");
   int p = pfinder(itr->pools, pool);
   check(p != -1, "Pool not found");
   action(permission_level{pool, "cpu"_n}, "eosio"_n,
            "refund"_n, std::make_tuple(pool))
         .send();
}

ACTION cpuloanpool::claimrefund(){
   require_auth(get_self());
   auto itr = pool_t.begin();
   check(itr != pool_t.end(), "Pool Table Not found");
   for (auto it : itr->pools){
      asset userBal = token::get_balance("eosio.token"_n,it.pool,WAX_S.code());
      if(userBal.amount > 0)
         action(permission_level{it.pool, "cpu"_n}, "eosio.token"_n, "transfer"_n,
                     std::make_tuple(it.pool, get_self(), userBal, std::string("")))
                  .send();
   }
}

void cpuloanpool::inlineunstak(const name account)
{
   auto stakes = staking.require_find(account.value, "Staking Entry Not Found");
   action(permission_level{stakes->pool_name, "cpu"_n}, "eosio"_n, "undelegatebw"_n,
      std::make_tuple(stakes->pool_name, stakes->account, asset(0, WAX_S), stakes->stakedcpu))
   .send();
   auto itr = pool_t.begin();
   check(itr != pool_t.end(), "Pool Table Not found");
   int p = pfinder(itr->pools, stakes->pool_name);
   check(p != -1, "Pool not found");
   vector<pair_name> temp = itr->pools;
   temp[p].stakedusers -= 1;
   pool_t.modify(itr, same_payer, [&](auto &v)
   { v.pools = temp; });
   stakes = staking.erase(stakes);
}

ACTION cpuloanpool::claimvote(const name account){
   require_auth(get_self());
   auto itr = pool_t.begin();
   check(itr != pool_t.end(), "Pool Table Not found");
   int p = pfinder(itr->pools, account);
   check(p != -1, "Pool not found");
   action(permission_level{account, "cpu"_n}, "eosio"_n,
      name("claimgbmvote"),
      std::make_tuple(account))
   .send();
}

int cpuloanpool::pfinder(vector<pair_name> pools, name pool)
{
   for (uint64_t i = 0; i < pools.size(); i++)
   {
      if (pools.at(i).pool == pool)
         return i;
   }
   return -1;
}

vector<string> cpuloanpool::getVector(char delim, string text)
{
   size_t start;
   size_t end = 0;
   std::vector<string> out;
   while ((start = text.find_first_not_of(delim, end)) != string::npos)
   {
      end = text.find(delim, start);
      out.push_back(text.substr(start, end - start));
   }
   return out;
}

name cpuloanpool::getPool(uint16_t days)
{
   auto itr = pool_t.begin();
   check(itr != pool_t.end(), "No Pool Exists");
   vector<pair_name> temp = itr->pools;
   auto it = config.begin();
   check(it != config.end(), "No Config Found");
   auto temp_id = (days % temp.size()) + it->index;
   auto id = temp_id > temp.size() ? temp_id % temp.size() : temp_id;
   id -= 1;
   return temp[id].pool;
}

void cpuloanpool::stakecpu(const name payer, const name receiver_account, const string type, const asset stake_cpu_asset, const uint64_t unstakeTime, const uint64_t days)
{
   auto stakes = staking.find(receiver_account.value);
   name pool = type == "new" ? getPool(days) : stakes->pool_name;
   if (type == "new")
   {
      check(stakes == staking.end(),"CPU Already Staked for this user");
      action(permission_level{get_self(), name("active")}, "eosio.token"_n,
               name("transfer"),
               std::make_tuple(get_self(), pool, stake_cpu_asset, "CPU for " + receiver_account.to_string()))
            .send();
      action(permission_level{pool, "cpu"_n}, "eosio"_n, "delegatebw"_n,
               std::make_tuple(pool, receiver_account, asset(0, WAX_S), stake_cpu_asset,
                              false))
            .send();
      staking.emplace(get_self(), [&](auto &v)
      {
         v.payer = payer;
         v.account = receiver_account;
         v.stakedcpu = stake_cpu_asset;
         v.pool_name = pool;
         v.unstakeTime = unstakeTime; });
      auto it = pool_t.begin();
      vector<pair_name> temp = it->pools;
      int p = pfinder(temp, pool);
      check(p != -1, "Pool Not found");
      temp[p].stakedusers += 1;
      pool_t.modify(it, get_self(), [&](auto &v)
      { v.pools = temp; });
   }
   else if (type == "renew")
   {
      uint32_t unstake_temp;
      uint32_t max_staking = config.begin()->max_days * config.begin()->unstakeSeconds;
      uint32_t proposed_renew = config.begin()->unstakeSeconds * days;
      if(current_time_point_sec().utc_seconds >= stakes->unstakeTime){
            if(proposed_renew >= max_staking) unstake_temp = max_staking;
            else unstake_temp = proposed_renew;
      }
      else {
         uint32_t remaining_unstake_seconds = stakes->unstakeTime - current_time_point_sec().utc_seconds;
         if((remaining_unstake_seconds + proposed_renew) > max_staking)
            unstake_temp = max_staking;
         else 
            unstake_temp = remaining_unstake_seconds + proposed_renew;
      }
      staking.modify(stakes, same_payer, [&](auto &v)
      {
         v.unstakeTime = current_time_point_sec().utc_seconds + unstake_temp;
         v.payer = payer; 
      });
   }
   else if (type == "add")
   {
      action{
         permission_level{get_self(), "active"_n},
         "eosio.token"_n,
         "transfer"_n,
         std::make_tuple(get_self(), pool, stake_cpu_asset, std::string("More"))
      }.send();
      action(permission_level{pool, "cpu"_n}, "eosio"_n, "delegatebw"_n,
         std::make_tuple(pool, receiver_account, asset(0, WAX_S), stake_cpu_asset,
                        false))
      .send();
      staking.modify(stakes, same_payer, [&](auto &v)
      { 
         v.stakedcpu += stake_cpu_asset;
         v.payer = payer;
      });
   }
}