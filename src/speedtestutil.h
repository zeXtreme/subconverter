#ifndef SPEEDTESTUTIL_H_INCLUDED
#define SPEEDTESTUTIL_H_INCLUDED

#include <string>
#include <yaml-cpp/yaml.h>

#include "misc.h"
#include "nodeinfo.h"

std::string vmessConstruct(std::string group, std::string remarks, std::string add, std::string port, std::string type, std::string id, std::string aid, std::string net, std::string cipher, std::string path, std::string host, std::string edge, std::string tls, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string ssrConstruct(std::string group, std::string remarks, std::string remarks_base64, std::string server, std::string port, std::string protocol, std::string method, std::string obfs, std::string password, std::string obfsparam, std::string protoparam, bool libev, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string ssConstruct(std::string group, std::string remarks, std::string server, std::string port, std::string password, std::string method, std::string plugin, std::string pluginopts, bool libev, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string socksConstruct(std::string group, std::string remarks, std::string server, std::string port, std::string username, std::string password, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string httpConstruct(std::string group, std::string remarks, std::string server, std::string port, std::string username, std::string password, bool tls, tribool scv = tribool());
std::string trojanConstruct(std::string group, std::string remarks, std::string server, std::string port, std::string password, std::string host, bool tlssecure, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string snellConstruct(std::string group, std::string remarks, std::string server, std::string port, std::string password, std::string obfs, std::string host, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
void explodeVmess(std::string vmess, const std::string &custom_port, nodeInfo &node);
void explodeSSR(std::string ssr, bool ss_libev, bool libev, const std::string &custom_port, nodeInfo &node);
void explodeSS(std::string ss, bool libev, const std::string &custom_port, nodeInfo &node);
void explodeTrojan(std::string trojan, const std::string &custom_port, nodeInfo &node);
void explodeQuan(std::string quan, const std::string &custom_port, nodeInfo &node);
void explodeShadowrocket(std::string kit, const std::string &custom_port, nodeInfo &node);
void explodeKitsunebi(std::string kit, const std::string &custom_port, nodeInfo &node);
/// Parse a link
void explode(std::string link, bool sslibev, bool ssrlibev, const std::string &custom_port, nodeInfo &node);
void explodeSSD(std::string link, bool libev, const std::string &custom_port, std::vector<nodeInfo> &nodes);
void explodeSub(std::string sub, bool sslibev, bool ssrlibev, const std::string &custom_port, std::vector<nodeInfo> &nodes);
int explodeConf(std::string filepath, const std::string &custom_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes);
int explodeConfContent(std::string content, const std::string &custom_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes);
bool chkIgnore(const nodeInfo &node, string_array &exclude_remarks, string_array &include_remarks);
void filterNodes(std::vector<nodeInfo> &nodes, string_array &exclude_remarks, string_array &include_remarks, int groupID);
bool getSubInfoFromHeader(std::string &header, std::string &result);
bool getSubInfoFromNodes(std::vector<nodeInfo> &nodes, string_array &stream_rules, string_array &time_rules, std::string &result);
bool getSubInfoFromSSD(std::string &sub, std::string &result);

#endif // SPEEDTESTUTIL_H_INCLUDED
