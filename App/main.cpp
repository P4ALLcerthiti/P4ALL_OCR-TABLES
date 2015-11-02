#include "ocrApp.h"
#include <QtWidgets/QApplication>

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
