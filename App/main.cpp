#define _SERVICE

#ifndef _SERVICE
#include "ocrApp.h"
#include <QtWidgets/QApplication>
#endif
#include <iostream>

#ifdef _SERVICE //dont forget to change the subsystem
#include <Windows.h>
#include "../OCR_TABLES/ocr_tabs.h"

int main(int argc, char *argv[])
{
	if (argc>1) 
	{
		std::string filename(argv[1]);
		if (filename.empty()) return -1;
		char buffer[MAX_PATH];
		GetModuleFileNameA( NULL, buffer, MAX_PATH );
		std::string::size_type pos = std::string( buffer ).find_last_of( "\\/" );
		std::string currentPath = std::string( buffer ).substr( 0, pos);
		pos = currentPath.find_last_of( "\\/" );
		currentPath = std::string( buffer ).substr( 0, pos);
		filename = currentPath + "\\" + filename;
		ocr_tabs tab;
		if (((filename.find(".pdf")!=std::string::npos) && (tab.pdf2html(filename))) ||  tab.img2html(filename) ) return 1;
		else return -1;
	}
	return 0;
}

#else

#ifdef _CONSOLE
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
#else 
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char*, int nShowCmd)
{
	int argc=0;
	QApplication a(argc,0);
#endif

	ocrApp w;
	//w.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	w.show();
	return a.exec();
}

#endif
