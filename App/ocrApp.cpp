#include "ocrApp.h"
#include "windows.h"
#include <Shlobj.h>
#include "QtWidgets\QFileDialog"


#pragma comment(lib,"Shell32.lib")

#pragma warning( disable : 4018 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )

ocrApp::ocrApp(QWidget *parent)
{
	ui.setupUi(this);
	connect(ui.buttonLoad, SIGNAL(clicked(bool)), this, SLOT(loadBt()));
	connect(ui.buttonShow, SIGNAL(clicked(bool)), this, SLOT(showBt()));

	tab = NULL;
	tab = new ocr_tabs();

}
//////////////////////////////////////////////
ocrApp::~ocrApp()
{
	if (tab!=NULL) delete tab;
}
//////////////////////////////////////////////
bool ocrApp::loadBt()
{
	ui.buttonShow->setEnabled(false);
	filename = QFileDialog::getOpenFileName(this, tr("Load Filename"), "", "Images (*.png *.bmp *.jpg *.jpeg);;PDF (*.pdf)");
	if (filename.isEmpty()) return false;

	ui.lineFilename->setText(filename);
	QApplication::processEvents();

	if (filename.endsWith(".pdf")) 	
	{
		ui.browserDeb->append ("Processing PDF file, please wait\n");
		QApplication::processEvents();
		if (tab->pdf2html(filename.toStdString()) == false)
		{
			ui.browserDeb->append ("Error - file processing failed\n");
			return false;
		}
		else
		{
			ui.browserDeb->append ("PDF file processed succesfully\n");
			ui.browserDeb->append ("HTML file saved at " + filename + (".html\nPress SHOW to open it with the default browser\n"));
			ui.buttonShow->setEnabled(true);
			return true;
		}
	}
	else
	{
		ui.browserDeb->append ("Processing IMG file, please wait\n");
		QApplication::processEvents();
		if (tab->img2html(filename.toStdString()) == false)
		{
			ui.browserDeb->append ("Error - file processing failed\n");
			return false;
		}
		else
		{
			ui.browserDeb->append ("IMG file processed succesfully\n");
			ui.browserDeb->append ("HTML file saved at " + filename + (".html\nPress SHOW to open it with the default browser\n"));
			ui.buttonShow->setEnabled(true);
			return true;
		}
	}
}
//////////////////////////////////////////////
bool ocrApp::showBt()
{
	std::string filenameHtml = QString(filename + ".html").toStdString();
	ShellExecuteA(NULL, ("open"), (filenameHtml.c_str()), NULL, NULL, SW_SHOWNORMAL);
	return true;
}
}