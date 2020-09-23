#pragma once

#include <iostream>

namespace service_discovery {

const char ketcd_ip[] = "";
const int ketcd_port = 123;

/**
 *  Let the service able to be discover by other node
 */
class ServiceNode {
public:
    ServiceNode(const std::string &identifier);

private:
    void RetreiveBatsIP();

    // Register itself to etcd. (identifier, IP)
    void Register();
    void RenewRegister();

    // Loop, Renew it register frequently
    void InnerLoop();
};
}
