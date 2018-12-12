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

// #define EOS_SYMBOL symbol("EOS", 4)

// #define MIXER_SYMBOL symbol("MIXER", 4)
#define EOS_SYMBOL S(4, EOS)
#define MIXER_SYMBOL S(4, MIXER)

using eosio::action;
using eosio::asset;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using eosio::symbol_type;
using eosio::time_point_sec;
using eosio::transaction;
using eosio::unpack_action_data;

class dividen : public eosio::contract
{
  public:
    const uint64_t MAIN_ID = 1;

    dividen(account_name self) : eosio::contract(self),
                                 globalvars(_self, _self),
                                 accounts(_self, _self)
    {
    }

    // @abi action
    void initcontract()
    {
        require_auth(_self);
        auto globalvars_itr = globalvars.begin();
        eosio_assert(globalvars_itr == globalvars.end(), "Contract is init");

        globalvars.emplace(_self, [&](auto &g) {
            g.id = MAIN_ID;
            //g.earnings_per_share = 0;
            g.total_staked = asset(0, MIXER_SYMBOL);
            g.eos_pool = asset(0, EOS_SYMBOL);
        });
    }

    //@abi action
    void hi()
    {
    }
    // @abi action
    void stake(account_name from, asset staked)
    {
        require_auth(from);

        eosio_assert(staked.is_valid(), "invalid quantity");
        eosio_assert(staked.amount > 0, "must transfer positive quantity");

        eosio_assert(staked.symbol.name() == eosio::symbol_type(MIXER_SYMBOL).name(), "symbol precision mismatch");

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
        if (account == accounts.end())
        {
            accounts.emplace(_self, [&](auto &account) {
                account.to = from;
                account.balance = staked;
            });
        }
        else
        {
            accounts.modify(account, _self, [&](auto &g) {
                g.balance += staked;
            });
        }
    }

    // @abi action
    void unstake(account_name from, asset staked)
    {
        require_auth(from);

        eosio_assert(staked.is_valid(), "invalid quantity");
        eosio_assert(staked.amount > 0, "must transfer positive quantity");

        //uint64_t delta = staked.amount;

        auto globalvars_itr = globalvars.find(MAIN_ID);

        eosio_assert(globalvars_itr->total_staked.amount > staked.amount, "total staked must greater unstaked quantity");

        globalvars.modify(globalvars_itr, _self, [&](auto &g) {
            g.total_staked -= staked;
            //g.earnings_per_share = g.eos_pool / (g.total_staked - delta);
        });

        auto account = accounts.find(from);
        eosio_assert(account != accounts.end(), "account not exists!");
        accounts.modify(account, _self, [&](auto &g) {
            g.balance -= staked;
        });
    }

    // @abi action
    void claim(account_name from)
    {
        require_auth(from);

        auto globalvars_itr = globalvars.begin();
        eosio_assert(globalvars_itr != globalvars.end(), "Contract is not init");

        auto account = accounts.find(from);
        eosio_assert(account != accounts.end(), "account not exists!");

        action(
            permission_level{_self, N(active)},
            N(eosio.token),
            N(transfer),
            std::make_tuple(
                _self,
                N(mangocasino1),
                asset(0, symbol_type(S(4, EOS))),
                //asset(0.0001, symbol_type(S(4, EOS))),
                std::string("ggggg")))
            .send();
    }

    //监听
    void transfer(uint64_t sender, uint64_t receiver)
    {
        auto transfer_data = unpack_action_data<st_transfer>();
        if (transfer_data.from == _self || transfer_data.from == N(mangocasino1))
        {
            return;
        }
        //
        eosio_assert(transfer_data.quantity.is_valid(), "Invalid asset");
    }

    // @abi action
    void reset()
    {
        require_auth(_self);
        auto cur_acc_itr = accounts.begin();
        while (cur_acc_itr != accounts.end())
        {
            cur_acc_itr = accounts.erase(cur_acc_itr);
        }
        auto cur_gvar_itr = globalvars.begin();
        while (cur_gvar_itr != globalvars.end())
        {
            cur_gvar_itr = globalvars.erase(cur_gvar_itr);
        }
    }

  private:
    // @abi table globalvars i64
    struct globalvar
    {
        uint64_t id;
        asset total_staked; //总抵押
        asset eos_pool;     //奖金池eos

        uint64_t primary_key() const { return id; }

        EOSLIB_SERIALIZE(globalvar, (id)(total_staked)(eos_pool));
    };

    typedef eosio::multi_index<N(globalvars), globalvar> globalvars_index;

    // @abi table accountlist i64
    struct account
    {
        asset balance;   //个人抵押
        account_name to; //用户
        //uint64_t primary_key() const { return balance.symbol.name(); }
        uint64_t primary_key() const { return to; }
    };

    typedef eosio::multi_index<N(accountlist), account> accountlist;

    // taken from eosio.token.hpp
    struct st_transfer
    {
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
             (initcontract)(reset)(stake)(unstake)(claim)(transfer))
