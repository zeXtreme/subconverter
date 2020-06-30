#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <mutex>

#include <curl/curl.h>

#include "webget.h"
#include "version.h"
#include "misc.h"
#include "logger.h"

#ifdef _WIN32
#ifndef _stat
#define _stat stat
#endif // _stat
#endif // _WIN32

extern bool print_debug_info, serve_cache_on_fetch_fail;
extern int global_log_level;

typedef std::lock_guard<std::mutex> guarded_mutex;
std::mutex cache_rw_lock;

//std::string user_agent_str = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36";
std::string user_agent_str = "subconverter/" VERSION " cURL/" LIBCURL_VERSION;

static inline void curl_init()
{
    static bool init = false;
    if(!init)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        init = true;
    }
}

static int writer(char *data, size_t size, size_t nmemb, std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

static int dummy_writer(char *data, size_t size, size_t nmemb, void *writerData)
{
    /// dummy writer, do not save anything
    (void)data;
    (void)writerData;
    return size * nmemb;
}

static int size_checker(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if(dltotal > 1048576.0)
        return 1;
    return 0;
}

static inline void curl_set_common_options(CURL *curl_handle, const char *url, long max_file_size = 1048576L)
{
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, global_log_level == LOG_LEVEL_VERBOSE ? 1L : 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 20L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
    if(max_file_size)
        curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE, max_file_size);
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, size_checker);
}

//static std::string curlGet(const std::string &url, const std::string &proxy, std::string &response_headers, CURLcode &return_code, const string_map &request_headers)
static int curlGet(const FetchArgument argument, FetchResult &result)
{
    CURL *curl_handle;
    std::string *data = result.content, new_url = argument.url;
    struct curl_slist *list = NULL;
    defer(curl_slist_free_all(list);)
    long retVal = 0;

    curl_init();

    curl_handle = curl_easy_init();
    if(argument.proxy.size())
    {
        if(startsWith(argument.proxy, "cors:"))
        {
            list = curl_slist_append(list, "X-Requested-With: subconverter " VERSION);
            new_url = argument.proxy.substr(5) + argument.url;
        }
        else
            curl_easy_setopt(curl_handle, CURLOPT_PROXY, argument.proxy.data());
    }
    curl_set_common_options(curl_handle, new_url.data());

    if(argument.request_headers)
    {
        for(auto &x : *argument.request_headers)
        {
            if(toLower(x.first) != "user-agent")
                list = curl_slist_append(list, (x.first + ": " + x.second).data());
        }
    }
    if(list)
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

    if(result.content)
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, result.content);
    }
    else
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, dummy_writer);
    if(result.response_headers)
    {
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, writer);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, result.response_headers);
    }
    else
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, dummy_writer);

    unsigned int fail_count = 0, max_fails = 1;
    while(true)
    {
        *result.status_code = curl_easy_perform(curl_handle);
        if(*result.status_code == CURLE_OK || max_fails >= fail_count)
            break;
        else
            fail_count++;
    }

    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);
    curl_easy_cleanup(curl_handle);

    if(data)
    {
        if(*result.status_code != CURLE_OK || retVal != 200)
            data->clear();
        data->shrink_to_fit();
    }

    return *result.status_code;
}

// data:[<mediatype>][;base64],<data>
static std::string dataGet(const std::string &url)
{
    if (!startsWith(url, "data:"))
        return std::string();
    std::string::size_type comma = url.find(',');
    if (comma == std::string::npos || comma == url.size() - 1)
        return std::string();

    std::string data = UrlDecode(url.substr(comma + 1));
    if (endsWith(url.substr(0, comma), ";base64")) {
        return urlsafe_base64_decode(data);
    } else {
        return data;
    }
}

std::string buildSocks5ProxyString(const std::string &addr, int port, const std::string &username, const std::string &password)
{
    std::string authstr = username.size() && password.size() ? username + ":" + password + "@" : "";
    std::string proxystr = "socks5://" + authstr + addr + ":" + std::to_string(port);
    return proxystr;
}

std::string webGet(const std::string &url, const std::string &proxy, unsigned int cache_ttl, std::string *response_headers, string_map *request_headers)
{
    int return_code = 0;
    std::string content;

    FetchArgument argument {url, proxy, request_headers, cache_ttl};
    FetchResult fetch_res {&return_code, &content, response_headers};

    if (startsWith(url, "data:"))
        return dataGet(url);
    // cache system
    if(cache_ttl > 0)
    {
        md("cache");
        const std::string url_md5 = getMD5(url);
        const std::string path = "cache/" + url_md5, path_header = path + "_header";
        struct stat result;
        if(stat(path.data(), &result) == 0) // cache exist
        {
            time_t mtime = result.st_mtime, now = time(NULL); // get cache modified time and current time
            if(difftime(now, mtime) <= cache_ttl) // within TTL
            {
                writeLog(0, "CACHE HIT: '" + url + "', using local cache.");
                guarded_mutex guard(cache_rw_lock);
                if(response_headers)
                    *response_headers = fileGet(path_header, true);
                return fileGet(path, true);
            }
            writeLog(0, "CACHE MISS: '" + url + "', TTL timeout, creating new cache."); // out of TTL
        }
        else
            writeLog(0, "CACHE NOT EXIST: '" + url + "', creating new cache.");
        //content = curlGet(url, proxy, response_headers, return_code); // try to fetch data
        curlGet(argument, fetch_res);
        if(return_code == CURLE_OK) // success, save new cache
        {
            guarded_mutex guard(cache_rw_lock);
            fileWrite(path, content, true);
            if(response_headers)
                fileWrite(path_header, *response_headers, true);
        }
        else
        {
            if(fileExist(path) && serve_cache_on_fetch_fail) // failed, check if cache exist
            {
                writeLog(0, "Fetch failed. Serving cached content."); // cache exist, serving cache
                guarded_mutex guard(cache_rw_lock);
                content = fileGet(path, true);
                if(response_headers)
                    *response_headers = fileGet(path_header, true);
            }
            else
                writeLog(0, "Fetch failed. No local cache available."); // cache not exist or not allow to serve cache, serving nothing
        }
        return content;
    }
    //return curlGet(url, proxy, response_headers, return_code);
    curlGet(argument, fetch_res);
    return content;
}

int curlPost(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData)
{
    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *list = NULL;
    long retVal = 0;

    curl_init();
    curl_handle = curl_easy_init();
    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    for(const std::string &x : request_headers)
        list = curl_slist_append(list, x.data());

    curl_set_common_options(curl_handle, url.data(), 0L);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, retData);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
    if(proxy.size())
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    res = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);

    if(res == CURLE_OK)
    {
        res = curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);
    }

    curl_easy_cleanup(curl_handle);

    return retVal;
}

int webPost(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData)
{
    return curlPost(url, data, proxy, request_headers, retData);
}

int curlPatch(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData)
{
    CURL *curl_handle;
    CURLcode res;
    long retVal = 0;
    struct curl_slist *list = NULL;

    curl_init();

    curl_handle = curl_easy_init();

    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    for(const std::string &x : request_headers)
        list = curl_slist_append(list, x.data());

    curl_set_common_options(curl_handle, url.data(), 0L);
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, retData);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
    if(proxy.size())
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    res = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if(res == CURLE_OK)
    {
        res = curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);
    }

    curl_easy_cleanup(curl_handle);

    return retVal;
}

int webPatch(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData)
{
    return curlPatch(url, data, proxy, request_headers, retData);
}

int curlHead(const std::string &url, const std::string &proxy, const string_array &request_headers, std::string &response_headers)
{
    CURL *curl_handle;
    CURLcode res;
    long retVal = 0;
    struct curl_slist *list = NULL;

    curl_init();

    curl_handle = curl_easy_init();

    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    for(const std::string &x : request_headers)
        list = curl_slist_append(list, x.data());

    curl_set_common_options(curl_handle, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
    if(proxy.size())
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    res = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if(res == CURLE_OK)
        res = curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);

    curl_easy_cleanup(curl_handle);

    return retVal;
}

int webHead(const std::string &url, const std::string &proxy, const string_array &request_headers, std::string &response_headers)
{
    return curlHead(url, proxy, request_headers, response_headers);
}
