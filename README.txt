The functioning of all actions and functions within the smart contract is as follow:

 1). Action: setconfig
 
Requires the authorization of the contract itself (get_self())
Purpose: Sets the configuration parameters for the CPU loan pool. It modifies an existing configuration if it exists, otherwise creates a new configuration.
If an existing configuration is modified, the config.modify function is called to update the values.
If a new configuration is created, the config.emplace function is called to insert a new configuration entry.

 2). Action: setindex

Requires the authorization of the contract itself (get_self())
Purpose: Updates the index value in the configuration. This index value is used to determine the current pool to be used for staking based on the number of days.

 3). Action: regpool

Requires the authorization of the contract itself (get_self())
Purpose: Registers a new pool in the pool table. If the pool table is empty, it creates a new entry with the provided pool name. If the pool table already exists, it appends the new pool name to the existing list of pools.
 
 4). Action: unregpool

Requires the authorization of the contract itself (get_self())
Purpose: Unregisters a pool from the pool table. It removes the specified pool name from the list of pools in the pool table.

5). Function: feepayment

Parameters: from, to, quantity, memo
Purpose: Handles the fee payment made by a user. The action checks the memo field to determine the type of fee payment (add, renew, or new) and performs the necessary calculations. It then calls the stakecpu function to stake the appropriate amount of CPU based on the fee payment.

     ->New:

When a user makes a "new" fee payment, it means they are staking a certain amount of WAX tokens to obtain CPU loan tokens for the first time.
The feepayment action handles this type of fee payment by checking the memo field to identify it as a "new" payment.
The getAmount function is called to calculate the stake amount of CPU tokens based on the fee and the number of days specified in the configuration.
The stakecpu function is then invoked to stake the calculated amount of CPU tokens from the user's account.

     ->Add:

If a user already has an existing stake and wants to add more WAX tokens to their stake, they make an "add" fee payment.
Similar to the "new" fee payment, the feepayment action identifies this type of payment by checking the memo field.
The getAddAmount function is used to calculate the stake amount of CPU tokens based on the fee and the remaining unstake time of the user's existing stake, as well as the unstake seconds from the configuration.
Again, the stakecpu function is called to stake the calculated amount of CPU tokens from the user's account.

     ->Renew:

When a user wants to renew their existing stake by extending the staking period, they make a "renew" fee payment.
The feepayment action detects this type of payment by examining the memo field.
The getRenew_Fees function is employed to calculate the renewal fees for the user's staked CPU tokens based on their account and the number of days specified.
The stakecpu function is utilized to stake the renewal fees, which keeps the staked CPU tokens locked for an extended period.

 6). Function: getAmount

Parameters: fee, days
Returns: float
Purpose: Calculates the stake amount of CPU tokens based on the given fee and number of days. It uses the fee rate from the configuration to determine the stake amount.

 7). Function: getRenew_Fees

Parameters: account, days
Returns: float
Purpose: Calculates the renewal fees for a user's staked CPU tokens based on the account and number of days. It considers the stake amount, fee rate, and renewal discount from the configuration to determine the renewal fees.

 8). Function: getAddAmount

Parameters: account, fee
Returns: float
Purpose: Calculates the stake amount of CPU tokens when a user adds more WAX tokens to an existing stake. It considers the remaining unstake time, fee rate, and unstake seconds from the configuration to determine the stake amount.

 9). Action: checkrefund

Requires the authorization of the contract itself (get_self())
Purpose: Checks for and performs inline unstaking for users in a specific pool whose unstake time has passed. It iterates through the staking table and calls the inlineunstak function for each user.

 10). Action: reset

Parameters: table
Requires the authorization of the contract itself (get_self())
Purpose: Resets specific tables in the contract.

 11). Function: stakecpu

Parameters: account, to, amount, memo
Purpose: Stakes the given amount of CPU tokens from the user's account to the specified recipient (to). It updates the staking table with the stake details.

 12). Function: inlineunstak

Parameters: account
Purpose: Performs inline unstaking for a specific user. It calculates the unstake time, removes the user's stake from the staking table, and transfers the staked CPU tokens back to the user's account.

 13). Action: Adminstake
 
Purpose: Checks the value of the type parameter to ensure it is either "add," "renew," or "new." Otherwise, it throws an error indicating an invalid type. then stakes CPU resources for a specified account with different types of stakes. 

 14). Action: Adminunstake
 
Requires the authorization of the contract itself (get_self()) to execute.
Purpose: It is used to unstake CPU resources from a specified account. Calls the inlineunstak() function, passing the specified account.

 15). Action: waxrefund
 
Requires the authorization of the contract itself (get_self()) to execute.
Purpose: It is used to request a refund for a specified pool account. Executes the eosio::refund action

 16). Action: claimrefund
 
Requires the authorization of the contract itself (get_self()) to execute.
Purpose: Retrieves the balance (userBal) of the WAX token for the corresponding pool account (it.pool).
If the balance (userBal.amount) is greater than zero, executes the eosio.token::transfer action.
The transfer action transfers the balance of the WAX token from the pool account (it.pool) to the contract's account.

 17). Action: claimvote
 
Requires the authorization of the contract itself (get_self()) to execute.
Purpose: It is used to claim a vote for a specified account by executing the eosio::claimgbmvote action. 

