#pragma once

#include <iostream>
#include <ostream>

#include "curl_easy.h"
#include "curl_exception.h"
#include "curl_ios.h"
#include "curl_exception.h"

#include "NaString.h"

using std::cout;
using std::endl;
using std::ostringstream;

using curl::curl_easy;
using curl::curl_ios;
using curl::curl_easy_exception;
using curl::curlcpp_traceback;

class NaCurl
{
public:
	NaCurl();
	~NaCurl();

	NaString Post(NaString strUrl, NaString strBody = L"");
	NaString Put(NaString strUrl, NaString strBody = L"");
	bool Get(NaString strUrl, char **outBuf, long &lSize);

	unsigned long GetLastError();
	NaString GetLastErrorMessage();

	size_t write_data(void * ptr, size_t size, size_t nmemb, void * stream);
	static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);

	ostringstream m_ostrOutput;
	curl_easy *m_pCurlEasy;

	unsigned long m_lLastError;
	NaString m_strLastError;

protected:
	void ClearLastError();
};