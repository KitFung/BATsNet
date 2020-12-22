#pragma once

namespace service_discovery {

/**
 * The expected endpoint for the BAT query service
 */
const char kBATs_ip[] = "10.0.0.100";
// const char kBATs_ip[] = "10.42.0.100";
const int kBATs_qport = 3314;

/**
 * The expected endpoint for the BAT service discovery registry
 */
const char ketcd_src[] = "http://10.0.0.100:2379";
// const char ketcd_src[] = "http://10.42.0.100:2379";
const char klocal_etcd_src[] = "http://127.0.0.1:2379";

} // namespace service_discovery
