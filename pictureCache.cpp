#include "pictureCache.h"
#include <Poco/Format.h>
#include <Poco/URIStreamOpener.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include "plexSdOsd.h"
#include "PVideo.h"
#include "Directory.h"
#include "plexgdm.h"
#include "PlexServer.h"

using Poco::URIStreamOpener;
using Poco::StreamCopier;
using Poco::Path;
using Poco::Exception;

cPictureCache::cPictureCache()
{
	m_cacheDir = cPlugin::CacheDirectory(PLUGIN_NAME_I18N);

	Poco::Path path(m_cacheDir);
	Poco::File f(path.toString());
	f.createDirectories();
	m_bAllInvalidated = false;
}

void cPictureCache::Action()
{
	while(Running()) {
		while (m_qImagesToLoad.size() > 0) {
			CacheInfo info = m_qImagesToLoad.front();
			m_qImagesToLoad.pop_front();

			std::string transcodeUri = TranscodeUri(info.uri, info.width, info.height);
			std::string file = FileName(info.uri, info.width);
			bool ok = true;
			if(!Cached(info.uri, info.width)) {
				ok = DownloadFileAndSave(transcodeUri, file);
			}
			if(ok) {
				LOCK_THREAD;
				if (!m_bAllInvalidated && info.onCached && info.calle && info.calle->IsVisible()) {
					info.onCached(info.calle);
				}
			}
			cCondWait::SleepMs(5);
		}
		cCondWait::SleepMs(100);
	}
}

bool cPictureCache::DownloadFileAndSave(std::string Uri, std::string localFile)
{
	try {
		Poco::URI fileUri(Uri);
		plexclient::PlexServer* pServer = plexclient::plexgdm::GetInstance().GetServer(fileUri.getHost(), fileUri.getPort());

		Poco::Net::HTTPResponse response;
		bool ok;
		std::istream &rs = pServer->MakeRequest(response, ok, fileUri.getPathAndQuery());

		if (!ok)
			return false;

		std::string type = response.getContentType();
		std::string fileName = localFile;

		// check filetype
		char buffer[2];
		rs.read(buffer, 2);
		bool isPng = false;
		
		if(buffer[0] == char(0x89) && buffer[1]  == char(0x50) ) {
			isPng = true;
		}
		rs.putback(buffer[1]);
		rs.putback(buffer[0]);

		if(isPng) {
			fileName += ".png";
		} else if(type == "image/jpeg" || fileUri.getPathAndQuery().find(".jpg") != std::string::npos) {
			fileName += ".jpg";
		}

		Poco::Path p(fileName);
		Poco::File f(p.makeParent().toString());
		f.createDirectories();

		std::ofstream outFile;
		outFile.open(fileName);
		Poco::StreamCopier::copyStream(rs, outFile);
		outFile.close();
		return true;
	} catch (Poco::Exception &exc) {
		return false;
	}

}

std::string cPictureCache::FileName(std::string uri, int width)
{
	Poco::URI u(uri);
	plexclient::PlexServer* pServer = plexclient::plexgdm::GetInstance().GetServer(u.getHost(), u.getPort());
	std::string uuid = "default";
	if(pServer && !pServer->GetUuid().empty()) {
		uuid = pServer->GetUuid();
	}
	std::string file = Poco::format("%s/%s_%d", uuid, u.getPathAndQuery(), width);

	Poco::Path path(m_cacheDir);
	path.append(file);

	return path.toString();
}

std::string cPictureCache::TranscodeUri(std::string uri, int width, int height)
{
	// We have to transform the uri a little...
	// httpX://ServerUri/.../queryUri=localhost
	
	Poco::URI u(uri);
	auto plServer = plexclient::plexgdm::GetInstance().GetServer(u.getHost(), u.getPort());
	u.setHost("127.0.0.1"); // set to localhost and http only!
	u.setScheme("http");
	
	Poco::URI serverUri(plServer->GetUri());
	serverUri.setPath("/photo/:/transcode");
	serverUri.addQueryParameter("width", std::to_string(width));
	serverUri.addQueryParameter("height", std::to_string(height));
	serverUri.addQueryParameter("url", u.toString());

	return serverUri.toString();
}

bool cPictureCache::Cached(std::string uri, int width)
{
	return Poco::File(FileName(uri, width) + ".jpg").exists() || Poco::File(FileName(uri, width) + ".png").exists();
}

std::string cPictureCache::GetPath(std::string uri, int width, int height, bool& cached, std::function<void(cGridElement*)> OnCached, cGridElement* calle)
{
	m_bAllInvalidated = false;
	cached = Cached(uri, width);
	std::string file = FileName(uri, width);
	if(cached) {
		if(Poco::File(FileName(uri, width) + ".jpg").exists()) {
			file += ".jpg";
		} else if(Poco::File(FileName(uri, width) + ".png").exists()) {
			file += ".png";
		}
		
		return file;
	} else {
		CacheInfo info(uri, width, height, OnCached, calle);
		m_qImagesToLoad.push_back(info);
	}

	return file;
}

void cPictureCache::Stop()
{
	Cancel();
}

void cPictureCache::Remove(cGridElement* element)
{
	if(!element) return;
	if (auto video = dynamic_cast<plexclient::Video*>(element)) {
		Remove(video->ThumbUri());
		Remove(video->ArtUri());
	}
}

void cPictureCache::Remove(std::string uri)
{
	LOCK_THREAD;
	for(std::deque<CacheInfo>::iterator it = m_qImagesToLoad.begin() ; it != m_qImagesToLoad.end(); ++it) {
		if(it->uri == uri) {
			m_qImagesToLoad.erase(it);
			return;
		}
	}
}

void cPictureCache::RemoveAll()
{
	LOCK_THREAD;
	m_bAllInvalidated = true;
	m_qImagesToLoad.clear();
}
