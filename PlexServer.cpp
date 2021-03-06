#include <vdr/tools.h>
#include "PlexServer.h"
#include "Config.h"
#include "plex.h"

namespace plexclient
{
	
PlexServer::PlexServer(std::string ip, int port)
{
	m_httpSession = NULL;
	
	Poco::URI uri;
	uri.setHost(ip);
	uri.setPort(port);
	uri.setScheme("http");
	m_uri = uri.toString();
	Offline = false;
}

PlexServer::PlexServer(std::string data, std::string ip)
{
	m_httpSession = NULL;
	ParseData(data, ip);
}

PlexServer::PlexServer(std::string uri, std::string name, std::string uuid, std::string accessToken, bool owned, bool local)
{
	m_httpSession = NULL;
	m_sServerName = name;
	m_sUuid = uuid;
	m_nOwned = owned;
	m_bLocal = local;
	m_authToken = accessToken;
	Offline = false;
	m_uri = uri;
}

PlexServer::~PlexServer()
{
	delete m_httpSession;
	m_httpSession = NULL;
}

void PlexServer::ParseData(std::string data, std::string ip)
{
	int port = 0;
	std::istringstream f(data);
	std::string s;
	Offline = false;
	while(std::getline(f, s)) {
		int pos = s.find(':');
		if(pos > 0) {
			std::string name = Poco::trim(s.substr(0, pos));
			std::string val = Poco::trim(s.substr(pos+1));
			if(name == "Content-Type") {
				m_sContentType = val;
			} else if (name == "Resource-Identifier") {
				m_sUuid = val;
			} else if (name == "Name") {
				m_sServerName = val;
			} else if (name == "Port") {
				port = atoi(val.c_str());
			} else if (name == "Updated-At") {
				m_nUpdated = atol(val.c_str());
			} else if (name == "Version") {
				m_sVersion = val;
			}
		}
	}
	
	m_bLocal = true;
	
	Poco::URI uri;
	
	uri.setHost(ip);
	uri.setPort(port);
	uri.setScheme("http");
	
	m_uri = uri.toString();
}

std::string PlexServer::GetHost()
{
	Poco::URI uri(m_uri);
	return uri.getHost();
}

int PlexServer::GetPort()
{
	Poco::URI uri(m_uri);
	return uri.getPort();
}

Poco::Net::HTTPClientSession* PlexServer::GetClientSession()
{
	Poco::URI uri(m_uri);
	if(m_httpSession == NULL) {
		if(uri.getScheme().find("https") != std::string::npos) {
			m_httpSession = new Poco::Net::HTTPSClientSession(uri.getHost(), uri.getPort());
		}
		else {
			m_httpSession = new Poco::Net::HTTPClientSession(uri.getHost(), uri.getPort());
		}
	}
	m_httpSession->setTimeout(Poco::Timespan(5, 0)); // set 5 seconds Timeout
	return m_httpSession;
}

std::istream& PlexServer::MakeRequest(Poco::Net::HTTPResponse& response, bool& ok, std::string path, const std::map<std::string, std::string>& queryParameters)
{
	Poco::URI uri(path);
	// Create a request with an optional query
	if(queryParameters.size()) {
		for (auto const& pair : queryParameters) {
			// addQueryParameter does the encode already
			uri.addQueryParameter(pair.first, pair.second);
		}
	}
	Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPathAndQuery(), Poco::Net::HTTPMessage::HTTP_1_1);

	request.add("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.52 Safari/537.17");
	request.add("X-Plex-Client-Identifier", Config::GetInstance().GetUUID());
	request.add("X-Plex-Device", "PC");
	request.add("X-Plex-Device-Name", Config::GetInstance().GetHostname());
	request.add("X-Plex-Language", Config::GetInstance().GetLanguage());
	request.add("X-Plex-Model", "Linux");
	request.add("X-Plex-Platform", "VDR");
	request.add("X-Plex-Product", "plex for vdr");
	request.add("X-Plex-Provides", "player");
	request.add("X-Plex-Version", VERSION);

	if(Config::GetInstance().UsePlexAccount && !GetAuthToken().empty()) {
		// Add PlexToken to Header
		request.add("X-Plex-Token", GetAuthToken());
	}
	bool excep = false;
	try {
		GetClientSession()->sendRequest(request);
	} catch (Poco::TimeoutException &exc) {
		esyslog("[plex] Timeout: %s", path.c_str());
		ok = false;
		excep = true;
	} catch (Poco::Exception &exc) {
		esyslog("[plex] Oops Exception: %s", exc.displayText().c_str());
		ok = false;
		excep = true;
	}
	if(!excep) { 
		std::istream& stream = GetClientSession()->receiveResponse(response);
		ok = response.getStatus() == 200;
		return stream;
	}
	static std::stringstream* ss;
	return *ss;
	
}

std::string PlexServer::GetUri()
{
	return  m_uri;
}

}
