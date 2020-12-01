#pragma once

namespace service_discovery {

const char kBATs_ip[] = "10.0.0.100";
// const char kBATs_ip[] = "10.42.0.100";
const int kBATs_qport = 3314;

const char ketcd_src[] = "http://10.0.0.100:2379";
// const char ketcd_src[] = "http://10.42.0.100:2379";
const char klocal_etcd_src[] = "http://127.0.0.1:2379";

} // namespace service_discovery
