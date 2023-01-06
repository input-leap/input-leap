/*  InputLeap -- mouse and keyboard sharing utility

    InputLeap is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) InputLeap developers.
*/

#include "arch/unix/ArchInternetUnix.h"

#include "arch/XArch.h"
#include "common/Version.h"
#include "base/Log.h"

#include <sstream>
#include <curl/curl.h>

class CurlFacade {
public:
    CurlFacade();
    ~CurlFacade();
    std::string get(const std::string& url);
    std::string urlEncode(const std::string& url);

private:
    CURL*                m_curl;
};

//
// ArchInternetUnix
//

std::string ArchInternetUnix::get(const std::string& url)
{
    CurlFacade curl;
    return curl.get(url);
}

std::string ArchInternetUnix::urlEncode(const std::string& url)
{
    CurlFacade curl;
    return curl.urlEncode(url);
}

//
// CurlFacade
//

static size_t
curlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    (static_cast<std::string*>(userp))->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

CurlFacade::CurlFacade() :
    m_curl(nullptr)
{
    CURLcode init = curl_global_init(CURL_GLOBAL_ALL);
    if (init != CURLE_OK) {
        throw XArch("CURL global init failed.");
    }

    m_curl = curl_easy_init();
    if (m_curl == nullptr) {
        throw XArch("CURL easy init failed.");
    }
}

CurlFacade::~CurlFacade()
{
    if (m_curl != nullptr) {
        curl_easy_cleanup(m_curl);
    }

    curl_global_cleanup();
}

std::string CurlFacade::get(const std::string& url)
{
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

    std::stringstream userAgent;
    userAgent << "Barrier ";
    userAgent << kVersion;
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, userAgent.str().c_str());

    std::string result;
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &result);

    CURLcode code = curl_easy_perform(m_curl);
    if (code != CURLE_OK) {
        LOG((CLOG_ERR "curl perform error: %s", curl_easy_strerror(code)));
        throw XArch("CURL perform failed.");
    }

    return result;
}

std::string CurlFacade::urlEncode(const std::string& url)
{
    char* resultCStr = curl_easy_escape(m_curl, url.c_str(), 0);

    if (resultCStr == nullptr) {
        throw XArch("CURL escape failed.");
    }

    std::string result(resultCStr);
    curl_free(resultCStr);

    return result;
}
