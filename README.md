cleos get table mtoken efg accountlist

cleos get table mtoken efg accountlist

cleos get table mtoken divid accountlist

cleos push action mtoken  create '["eosio","1000000000.0000 EOS"]' -p mtoken

cpp mtoken issue '["divid","100000.0000 EOS",""]' -p eosio

cpp mtoken issue '["abc","10000.0000 EOS",""]' -p eosio

cpp mtoken issue '["efg","10000.0000 EOS",""]' -p eosio

cleos push action mtoken issue '[ "divid", "100.0000 EOS", "mae_profit" ]' -p eosio

