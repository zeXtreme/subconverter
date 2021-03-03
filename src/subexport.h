#ifndef SUBEXPORT_H_INCLUDED
#define SUBEXPORT_H_INCLUDED

#include <string>
#include <vector>
#include <future>

#include "misc.h"
#include "ini_reader.h"
#include "nodeinfo.h"

enum ruleset_type
{
    RULESET_SURGE,
    RULESET_QUANX,
    RULESET_CLASH_DOMAIN,
    RULESET_CLASH_IPCIDR,
    RULESET_CLASH_CLASSICAL
};

struct ruleset_content
{
    std::string rule_group;
    std::string rule_path;
    std::string rule_path_typed;
    int rule_type = RULESET_SURGE;
    std::shared_future<std::string> rule_content;
    int update_interval = 0;
};

struct extra_settings
{
    bool enable_rule_generator = true;
    bool overwrite_original_rules = true;
    string_array rename_array;
    string_array emoji_array;
    bool add_emoji = false;
    bool remove_emoji = false;
    bool append_proxy_type = false;
    bool nodelist = false;
    bool sort_flag = false;
    bool filter_deprecated = false;
    bool clash_new_field_name = false;
    bool clash_script = false;
    std::string surge_ssr_path;
    std::string managed_config_prefix;
    std::string quanx_dev_id;
    tribool udp = tribool();
    tribool tfo = tribool();
    tribool skip_cert_verify = tribool();
    tribool tls13 = tribool();
    bool clash_classical_ruleset = false;
    std::string sort_script = "";
    std::string clash_proxies_style = "flow";
};

void rulesetToClash(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name);
void rulesetToSurge(INIReader &base_rule, std::vector<ruleset_content> &ruleset_content_array, int surge_ver, bool overwrite_original_rules, std::string remote_path_prefix);
void preprocessNodes(std::vector<nodeInfo> &nodes, const extra_settings &ext);

std::string netchToClash(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, bool clashR, const extra_settings &ext);
void netchToClash(std::vector<nodeInfo> &nodes, YAML::Node &yamlnode, const string_array &extra_proxy_group, bool clashR, const extra_settings &ext);
std::string netchToSurge(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, int surge_ver, const extra_settings &ext);
std::string netchToMellow(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
void netchToMellow(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
std::string netchToLoon(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
std::string netchToSSSub(std::string &base_conf, std::vector<nodeInfo> &nodes, const extra_settings &ext);
std::string netchToSingle(std::vector<nodeInfo> &nodes, int types, const extra_settings &ext);
std::string netchToQuanX(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
void netchToQuanX(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
std::string netchToQuan(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
void netchToQuan(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext);
std::string netchToSSD(std::vector<nodeInfo> &nodes, std::string &group, std::string &userinfo, const extra_settings &ext);

#endif // SUBEXPORT_H_INCLUDED
