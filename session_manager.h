#pragma once
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include "mutex_map.h"
#include "network_header.h"
#include "session.h"

class SessionManager {
    std::shared_ptr<MutexMap<SockAddr, std::shared_ptr<Session>>> ses_map;

public:
    SessionManager() {
        ses_map.reset(new MutexMap<SockAddr, std::shared_ptr<Session>>());
    }

    void callbackErase(SockAddr key) {
        auto it = ses_map.get()->find(key);
        if (it != ses_map.get()->end()) {
            ses_map.get()->erase(it); // If iterator is deleted then error?
        }
        printf("Session : (%u)\n", ses_map.get()->size());
    }

    void add(std::unique_ptr<TcpData> data) {

        if (data.get()->tcp_syn) {
            SockAddr src_addr = data.get()->src_sock;
            auto it = ses_map.get()->find(src_addr);

            if (it != ses_map.get()->end()) {
                /* TODO: Need to kill thread */
                it->second.get()->kill();
            }

            std::shared_ptr<Session> ses = std::make_shared<Session>(std::move(data),
                                                                     std::bind(&SessionManager::callbackErase, this, std::placeholders::_1));

            ses_map.get()->insert(src_addr, ses);

            /* Create thread */
            std::thread thr { &Session::process, ses };
            thr.detach();


        } else {
            auto it = ses_map.get()->find(data.get()->src_sock);

            if (it == ses_map.get()->end()) {
                return;
            }

            std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> ses_que = it->second->getQueue();
            ses_que.get()->push(move(data));


        }

    }

};
