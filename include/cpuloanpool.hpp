#include <eosio/eosio.hpp>
#include <token.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>
#include <stdint.h>
#include <string>
#include <math.h>
using namespace eosio;
using namespace std;

#define EOSIO name("eosio")

CONTRACT cpuloanpool : public contract
{
public:
   using contract::contract;
   static constexpr symbol WAX_S = symbol(symbol_code("WAX"), 8);

   struct pair_name
   {
      name pool;
      uint16_t stakedusers;
   };
   ACTION adminstake(const name account, const string type, const asset stake_cpu_asset, const uint64_t days);
   ACTION adminunstake(const name account);
   ACTION checkrefund(const name pool);
   ACTION claimvote(const name account);
   ACTION regpool(const name pool);
   ACTION reset(const string table);
   ACTION setconfig(const uint64_t unstakeSeconds,const float fee, const asset min_amount, const uint64_t min_days, const uint64_t max_days, const uint64_t renew_dis, const uint64_t renew_mult);
   ACTION setindex();
   [[eosio::on_notify("eosio.token::transfer")]] void feepayment(const name &from, const name &to, const asset &quantity, const string &memo);
   ACTION unregpool(const name pool);
   ACTION waxrefund(const name pool);
   ACTION claimrefund();

private:
   TABLE pools{
      uint16_t id;
      vector<pair_name> pools;
      auto primary_key() const { return id; }
   };

   TABLE config_data
   {
      uint16_t id;
      asset earnings;
      float fee_rate;
      asset min_amount;
      uint64_t min_days;
      uint64_t max_days;
      uint64_t unstakeSeconds;
      uint64_t renew_dis;
      uint64_t renew_mult;
      uint64_t index;
      auto primary_key() const { return id; }
   };

   TABLE staking_data
   {
      name account;
      uint64_t unstakeTime;
      asset stakedcpu;
      name payer;
      name pool_name;
      auto primary_key() const { return account.value; };
      uint64_t by_account() const { return pool_name.value; };
      uint64_t by_payer() const { return payer.value; };
   };

   typedef eosio::multi_index<"pools"_n, pools> pool_s;
   typedef eosio::multi_index<"config"_n, config_data> config_s;
   typedef eosio::multi_index<"staking"_n, staking_data,
                              indexed_by<name("account"), const_mem_fun<staking_data, uint64_t, &staking_data::by_account>>,
                              indexed_by<name("payer"), const_mem_fun<staking_data, uint64_t, &staking_data::by_payer>>>staking_s;

   staking_s staking = staking_s(get_self(), get_self().value);
   config_s config = config_s(get_self(), get_self().value);
   pool_s pool_t = pool_s(get_self(),get_self().value);

   vector<string> getVector(char delim, string text);
   void inlineunstak(const name account);
   float getAmount(uint64_t fee, uint64_t days);
   float getRenew_Fees(name account, uint64_t days);
   float getAddAmount(name account,uint64_t fee);
   int pfinder(vector<pair_name> pools, name pool);
   name getPool(uint16_t days);
   void stakecpu(const name payer, const name receiver_account, const string type, const asset stake_cpu_asset, const uint64_t unstakeTime, const uint64_t days);
};