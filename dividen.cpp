#include <utility>
#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include "eosio.token.hpp"


#define EOS_SYMBOL S(4, EOS)
#define PLAT_SYMBOL S(4,LKCH)

using eosio::action;
using eosio::asset;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using eosio::symbol_type;
using eosio::time_point_sec;
using eosio::transaction;
using eosio::unpack_action_data;

class dividen : public eosio::contract {
public:
    const uint64_t MAIN_ID = 1;

    dividen(account_name self) : eosio::contract(self),
                                 globalvars(_self, _self),
                                 accounts(_self, _self) {
    }

    // @abi action
    void initcontract() {
        require_auth(_self);
        auto globalvars_itr = globalvars.begin();
        eosio_assert(globalvars_itr == globalvars.end(), "Contract is init");

        globalvars.emplace(_self, [&](auto &g) {
            g.id = MAIN_ID;
            //g.earnings_per_share = 0;
            g.total_staked = asset(0, PLAT_SYMBOL);
            g.eos_pool = asset(0, EOS_SYMBOL);
        });
    }

    //@abi action
    void dobet(account_name player, account_name invite, asset bet) {
        print("dobet...", "\n");
        require_auth(N(luckychainme));
        eosio_assert(bet.is_valid(), "invalid quantity");
        eosio_assert(bet.amount > 0, "must transfer positive quantity");

        eosio_assert(bet.symbol.name() == eosio::symbol_type(EOS_SYMBOL).name(), "symbol precision mismatch");
        asset player_plat = asset(10 * bet.amount, PLAT_SYMBOL);
        asset invite_plat = asset(0.1 * bet.amount, PLAT_SYMBOL);
        print("bet.amount:", bet.amount, "\n");
        auto acc_plyer = accounts.find(player);
        auto acc_invite = accounts.find(invite);
        print("player_plat:", player_plat, "\n");
        print("invite_plat:", invite_plat, "\n");

         action(
                permission_level{_self, N(active)},
                N(luckychaintm),
                N(transfer),
                std::make_tuple(
                        _self ,
                        player,
                        player_plat,
                        //asset(0.0001, symbol_type(S(4, EOS))),
                        std::string("ggggg")))
                .send();

        action(
                permission_level{_self, N(active)},
                N(luckychaintm),
                N(transfer),
                std::make_tuple(
                        _self,
                        invite,
                        invite_plat,
                        //asset(0.0001, symbol_type(S(4, EOS))),
                        std::string("ggggg")))
                .send();

    }

    // @abi action
    void stake(account_name from, asset staked) {
        require_auth(from);

        eosio_assert(staked.is_valid(), "invalid quantity");
        eosio_assert(staked.amount > 0, "must transfer positive quantity");

        eosio_assert(staked.symbol.name() == eosio::symbol_type(PLAT_SYMBOL).name(), "symbol precision mismatch");

        //uint64_t delta = staked.amount;

        // eosio::print("delta:", delta, "\n");
        // auto globalvars_itr = globalvars.get(MAIN_ID);
        auto globalvars_itr = globalvars.find(MAIN_ID);
        // 质押更新
        globalvars.modify(globalvars_itr, _self, [&](auto &g) {
            g.total_staked += staked;
            //g.earnings_per_share = g.eos_pool / (delta + g.total_staked);
        });
        auto account = accounts.find(from);
        // eosio::print("account:", account, "n");
        if (account == accounts.end()) {
            accounts.emplace(_self, [&](auto &account) {
                account.to = from;
                account.balance = staked;
            });
        } else {
            accounts.modify(account, _self, [&](auto &g) {
                g.balance += staked;
            });
        }


        action(
                permission_level{from, N(active)},
                N(luckychaintk),
                N(transfer),
                std::make_tuple(
                        from,
                        _self,
                        staked,
                        std::string("ggggg")))
                .send();
    }

    // @abi action
    void unstake(account_name from, asset unstaked) {
        require_auth(from);

        eosio::transaction txn{};

        txn.actions.emplace_back(
                eosio::permission_level(from, N(active)),
                _self,
                N(temp),
                std::make_tuple(
                        from, unstaked));
        //TODO 赎回时长
        // txn.delay_sec = 60*60*24; //赎回时间24小时
        txn.delay_sec = 3; //赎回时间24小时
        //(sender_id, payer, replace_existed)
        txn.send(uint128_t(from) << 64 | current_time(), _self, true);
    }

    void temp(account_name from, asset unstaked) {
        print("temp","\n");
        require_auth(from);
        eosio_assert(unstaked.is_valid(), "invalid quantity");
        eosio_assert(unstaked.amount > 0, "must transfer positive quantity");

        //uint64_t delta = staked.amount;

        auto globalvars_itr = globalvars.find(MAIN_ID);

        eosio_assert(globalvars_itr->total_staked.amount >= unstaked.amount,
                     "total staked must greater unstaked quantity");

        globalvars.modify(globalvars_itr, _self, [&](auto &g) {
            g.total_staked -= unstaked;
            //g.earnings_per_share = g.eos_pool / (g.total_staked - delta);
        });

        auto account = accounts.find(from);
        eosio_assert(account != accounts.end(), "account not exists!");
        // accounts.modify(account, _self, [&](auto &g) {
        //     g.balance -= staked;
        // });

        if (account->balance.amount == unstaked.amount) {
            accounts.erase(account);
        } else {
            accounts.modify(account, _self, [&](auto &g) {
                g.balance -= unstaked;
            });
        }
        action(
                permission_level{_self, N(active)},
                N(luckychaintk),
                N(transfer),
                std::make_tuple(
                        _self,
                        from,
                        unstaked,
                        std::string("ggggg")))
                .send();
    }

    // @abi action
    void claim(account_name from) {
        require_auth(from);
        auto account = accounts.find(from);
        eosio_assert(account != accounts.end(), "account not exists!");
        auto globalvars_itr = globalvars.find(MAIN_ID);
        uint64_t account_stake = account->balance.amount;
        uint64_t total_stake = globalvars_itr->total_staked.amount;
        uint64_t total_pool = globalvars_itr->eos_pool.amount;
        uint64_t stake_profit = total_pool * account_stake / total_stake;
        print("account_stake:",account_stake,"\n");
        print("total_stake:",total_stake,"\n");
        print("total_pool:",total_pool,"\n");
        print("stake_profit:",stake_profit,"\n");
        if (stake_profit == 0) {
            return;
        }
        asset t_eos = asset(stake_profit, symbol_type(S(4, EOS)));
        globalvars.modify(globalvars_itr, _self, [&](auto &g) {
            g.eos_pool -= t_eos;
        });
        action(
                permission_level{_self, N(active)},
                N(eosio.token),
                N(transfer),
                std::make_tuple(
                        _self,
                        from,
                        t_eos,
                        std::string("ggggg")))
                .send();
    }

    inline void profit(asset quantity) {
        auto globalvars_itr = globalvars.find(MAIN_ID);

        if (globalvars_itr == globalvars.end()) {
            initcontract();
        } else {
            globalvars.modify(globalvars_itr, _self, [&](auto &g) {
                g.eos_pool += quantity;
            });
        }
    }

    //监听
    void transfer(uint64_t sender, uint64_t receiver) {
        auto transfer_data = unpack_action_data<st_transfer>();

        if (transfer_data.from != N(luckychainme)) {
            return;
        }
        //
        std::string mem_str = transfer_data.memo;
        asset quantity = transfer_data.quantity;

        eosio_assert(quantity.is_valid(), "Invalid asset");

        eosio_assert(quantity.amount > 0, "must transfer positive quantity");

        if (quantity.symbol.name() != eosio::symbol_type(EOS_SYMBOL).name()) {
            return;
        }
        eosio_assert(quantity.symbol.name() == eosio::symbol_type(EOS_SYMBOL).name(), "EOS symbol precision mismatch");
        eosio_assert(mem_str.size() <= 256, "memo has more than 256 bytes");

        eosio::print("mem_str:", mem_str, "n");

        //auto params = split(mem_str, ' ');
        //eosio_assert(params.size() >= 1, "error memo");

        if (mem_str == "make_profit") {
            //eosio_assert(quantity.contract == N(eosio.token), "must use true EOS to make profit");
            eosio_assert(quantity.symbol == EOS_SYMBOL, "must use EOS to make profit");
            // make_profit(quantity.quantity.amount);

            profit(quantity);

            return;
        }
    }

    // @abi action
    void reset() {
        require_auth(_self);
        auto cur_acc_itr = accounts.begin();
        while (cur_acc_itr != accounts.end()) {
            cur_acc_itr = accounts.erase(cur_acc_itr);
        }
        auto cur_gvar_itr = globalvars.begin();
        while (cur_gvar_itr != globalvars.end()) {
            cur_gvar_itr = globalvars.erase(cur_gvar_itr);
        }
    }

private:
    // @abi table globalvars i64
    struct globalvar {
        uint64_t id;
        asset total_staked; //总抵押
        asset eos_pool;     //奖金池eos

        uint64_t primary_key() const { return id; }

        EOSLIB_SERIALIZE(globalvar, (id)(total_staked)(eos_pool)
        );
    };

    typedef eosio::multi_index<N(globalvars), globalvar> globalvars_index;

    // @abi table accountlist i64
    struct account {
        asset balance;   //个人抵押
        account_name to; //用户
        //uint64_t primary_key() const { return balance.symbol.name(); }
        uint64_t primary_key() const { return to; }
    };

    typedef eosio::multi_index<N(accountlist), account> accountlist;

    // taken from eosio.token.hpp
    struct st_transfer {
        account_name from; //
        account_name to;
        asset quantity;
        std::string memo;
    };

    accountlist accounts;

    globalvars_index globalvars;
};

#define EOSIO_ABI_EX(TYPE, MEMBERS)                                                    \
    extern "C"                                                                         \
    {                                                                                  \
        void apply(uint64_t receiver, uint64_t code, uint64_t action)                  \
        {                                                                              \
            auto self = receiver;                                                      \
            if (code == self || code == N(eosio.token))                                \
            {                                                                          \
                if (action == N(transfer))                                             \
                {                                                                      \
                    eosio_assert(code == N(eosio.token), "Must transfer EOS");         \
                }                                                                      \
                TYPE thiscontract(self);                                               \
                switch (action)                                                        \
                {                                                                      \
                    EOSIO_API(TYPE, MEMBERS)                                           \
                }                                                                      \
                /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
            }                                                                          \
        }                                                                              \
    }

EOSIO_ABI_EX(dividen,
             (initcontract)(reset)(dobet)(stake)(unstake)(claim)(transfer)(temp))
