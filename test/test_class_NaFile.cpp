#include "stdafx.h"
#include <catch/catch.hpp>

#include <NaLib/NaFile.h>

TEST_CASE("NaFile::Load", "[.]") 
{
	auto pFile = NaFile::Load(L"D:\\temp\\file.ext", "rb");
	CHECK(pFile != nullptr);
	CHECK(pFile->m_hFile != nullptr);
	delete pFile;

	pFile = NaFile::Load(L"d:\\temp\\�ѱ�\\file.ext", "rb");
	CHECK(pFile != nullptr);
	CHECK(pFile->m_hFile != nullptr);
	delete pFile;

	pFile = NaFile::Load(L"d:\\temp\\�ѱ�\\����.Ȯ����", "rb");
	CHECK(pFile != nullptr);
	CHECK(pFile->m_hFile != nullptr);
	delete pFile;
}
