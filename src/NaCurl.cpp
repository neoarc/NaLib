#include "stdafx.h"
#include "NaCurl.h"

#include "NaDebug.h"

std::map<ostringstream *, void*> NaCurl::s_mapInstance;

NaCurl::NaCurl()
{
	curl_ios<ostringstream> writer(m_ostrOutput);
	writer.set_callback(NaCurl::write_callback);

	// Pass the writer to the easy constructor and watch the content returned in that variable!
	m_pCurlEasy = new curl_easy(writer);

	m_lLastError = 0;
	m_strLastError = L"";
	m_lDownloaded = 0;

	s_mapInstance.insert(
		std::pair<ostringstream*, void*>(&m_ostrOutput, this)
	);
}

NaCurl::~NaCurl()
{
	// #TODO CriticalSection needed?
	auto it = s_mapInstance.find((ostringstream*)&m_ostrOutput);
	if (it != s_mapInstance.end())
	{
		s_mapInstance.erase(it);
	}

	if (m_pCurlEasy)
	{
		delete m_pCurlEasy;
		m_pCurlEasy = nullptr;
	}
}

NaString NaCurl::Post(NaString strUrl, NaString strBody)
{
	NaDebugOut(L"URL: %ls\n", strUrl.wstr());

	// Add some option to the easy handle
	m_pCurlEasy->add<CURLOPT_URL>(strUrl.cstr());
	m_pCurlEasy->add<CURLOPT_FOLLOWLOCATION>(1L);
	m_pCurlEasy->add<CURLOPT_SSL_VERIFYPEER>(0);

	if (strBody.GetLength() > 0)
	{
		m_pCurlEasy->add<CURLOPT_POSTFIELDS>(strBody.cstr());
	}

	try
	{
		// Execute the request.
		ClearOutputStream();
		m_pCurlEasy->perform();
	}
	catch (curl_easy_exception error)
	{
		// If you want to get the entire error stack we can do:
		curlcpp_traceback errors = error.get_traceback();

		// Otherwise we could print the stack like this:
		error.print_traceback();

		// #TODO Exception handling

		/*
		std::for_each(curl_exception::traceback.begin(), curl_exception::traceback.end(), [](const curlcpp_traceback_object &value) {
		std::cout << "ERROR: " << value.first << " ::::: FUNCTION: " << value.second << std::endl;
		});
		*/

		//throw NaException(__FUNCTIONW__, __LINE__, L"NaCurl internal exception.");

		return L"";
	}

	// Let's print the stream content.
	const std::string str = m_ostrOutput.str();
	const char* cstr = str.c_str();

	NaString strRet = cstr;
	if (strRet.GetLength() > 80)
		NaDebugOut(L"RET: %ls ...(skip)\n", strRet.Left(80).wstr());
	else
		NaDebugOut(L"RET: %ls\n", strRet.wstr());

	return strRet;
}

NaString NaCurl::Put(NaString strUrl, NaString strBody)
{
	if (strBody.GetLength() == 0)
		return L"";

	NaDebugOut(L"URL: %ls\n", strUrl.wstr());

	// Add some option to the easy handle
	m_pCurlEasy->add<CURLOPT_URL>(strUrl.cstr());
	m_pCurlEasy->add<CURLOPT_FOLLOWLOCATION>(1L);
	m_pCurlEasy->add<CURLOPT_SSL_VERIFYPEER>(0);
	m_pCurlEasy->add<CURLOPT_CUSTOMREQUEST>("PUT");

	m_pCurlEasy->add<CURLOPT_POSTFIELDS>(strBody.cstr());

	try
	{
		// Execute the request.
		ClearOutputStream();
		m_pCurlEasy->perform();
	}
	catch (curl_easy_exception error)
	{
		// If you want to get the entire error stack we can do:
		curlcpp_traceback errors = error.get_traceback();
		// Otherwise we could print the stack like this:
		error.print_traceback();
	}

	// Let's print the stream content.
	const std::string str = m_ostrOutput.str();
	const char* cstr = str.c_str();

	NaString strRet = cstr;
	if (strRet.GetLength() > 80)
		NaDebugOut(L"RET: %ls ...(skip)\n", strRet.Left(80).wstr());
	else
		NaDebugOut(L"RET: %ls\n", strRet.wstr());

	return strRet;
}

bool NaCurl::Get(NaString strUrl, char **outBuf, long &lSize)
{
	NaDebugOut(L"URL: %ls\n", strUrl.wstr());

	// Add some option to the easy handle
	m_pCurlEasy->add<CURLOPT_URL>(strUrl.cstr());
	m_pCurlEasy->add<CURLOPT_FOLLOWLOCATION>(1L);
	m_pCurlEasy->add<CURLOPT_NOSIGNAL>(1L);
	m_pCurlEasy->add<CURLOPT_ACCEPT_ENCODING>("deflate");

	try
	{
		// Execute the request.
		ClearOutputStream();
		m_pCurlEasy->perform();
	}
	catch (curl_easy_exception error)
	{
		// If you want to get the entire error stack we can do:
		curlcpp_traceback errors = error.get_traceback();
		// Otherwise we could print the stack like this:
		error.print_traceback();

		return false;
	}

	// Let's print the stream content.
	const std::string str = m_ostrOutput.str();
	const char* cstr = str.c_str();

	if ((size_t)lSize != -1 && str.size() != (size_t)lSize)
	{
		m_strLastError.Format(L"Size mismatch: %d, %d", str.size(), lSize);
		return false;
	}
	else
	{
		lSize = str.size();
		if (*outBuf == nullptr)
			*outBuf = new char[lSize];
		memcpy(*outBuf, cstr, lSize);
	}

	return true;
}

bool NaCurl::UploadMultiPart(NaString strUrl, NaString strLocalFilePath, NaString strBody)
{
	if (strLocalFilePath.GetLength() == 0)
		return false;

	NaDebugOut(L"URL: %ls\n", strUrl.wstr());

	// Add some option to the easy handle
	m_pCurlEasy->add<CURLOPT_URL>(strUrl.cstr());
	m_pCurlEasy->add<CURLOPT_FOLLOWLOCATION>(1L);
	m_pCurlEasy->add<CURLOPT_SSL_VERIFYPEER>(0);
	
	/* Fill in the file upload field. This makes libcurl load data from
	the given file name when curl_easy_perform() is called. */
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	//struct curl_slist *headerlist = NULL;
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "file",
		CURLFORM_FILE, strLocalFilePath.cstr(),
		CURLFORM_END);

	if (strBody.GetLength() > 0)
	{
		curl_formadd(&formpost, &lastptr, 
			CURLFORM_COPYNAME, "text",
			CURLFORM_COPYCONTENTS, strBody.cstr(), CURLFORM_END);
	}

	m_pCurlEasy->add<CURLOPT_HTTPPOST>(formpost);

	try
	{
		// Execute the request.
		ClearOutputStream();
		m_pCurlEasy->perform();
	}
	catch (curl_easy_exception error)
	{
		// If you want to get the entire error stack we can do:
		curlcpp_traceback errors = error.get_traceback();
		// Otherwise we could print the stack like this:
		error.print_traceback();

		//////////////////////////////////////////////////////////////////////////
		m_strLastError = L"";

		auto pThis = this;
		auto traceback = error.get_traceback();
		std::for_each(traceback.begin(), traceback.end(),
			[&pThis](const curl::curlcpp_traceback_object &value)
		{
			pThis->m_strLastError.AppendFormat("E: %s, F: %s\n",
				value.first.c_str(), value.second.c_str()
			);
		});
		//////////////////////////////////////////////////////////////////////////
	}

	// Let's print the stream content.
	const std::string str = m_ostrOutput.str();
	const char* cstr = str.c_str();

	NaString strRet = cstr;
	if (strRet.GetLength() > 80)
		NaDebugOut(L"RET: %ls ...(skip)\n", strRet.Left(80).wstr());
	else
		NaDebugOut(L"RET: %ls\n", strRet.wstr());

	curl_formfree(formpost);
	//curl_slist_free_all(headerlist);

	return true;
}

unsigned long NaCurl::GetLastError()
{
	return m_lLastError;
}

NaString NaCurl::GetLastErrorMessage()
{
	return m_strLastError;
}


void NaCurl::ClearOutputStream()
{
	m_ostrOutput.str("");
	m_ostrOutput.clear();
	m_lDownloaded = 0;
}

void NaCurl::ClearLastError()
{
	m_lLastError = 0;
	m_strLastError = L"";
}

void NaCurl::SetCallback(std::function<void(size_t)> fnCallback)
{
	m_UserCallback = fnCallback;
}

void NaCurl::OnCallback(size_t added)
{
	if (m_UserCallback)
		m_UserCallback(added);
}

size_t NaCurl::write_callback(void * contents, size_t size, size_t nmemb, void * userp)
{
	const size_t added = (size * nmemb);

	auto it = s_mapInstance.find((ostringstream*)userp);
	if (it != s_mapInstance.end())
	{
		NaCurl* pCurl = (NaCurl*)it->second;
		std::string data((const char*)contents, (size_t)added);
		pCurl->m_ostrOutput << data;

		pCurl->m_lDownloaded += added;
		pCurl->OnCallback(added);
	}

	return size * nmemb;
}
