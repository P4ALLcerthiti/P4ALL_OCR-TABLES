#pragma once

#include "../OCR_TABLES/ocr_tabs.h"
#include "gui.h"

class ocrApp : public QMainWindow
{
	Q_OBJECT

public:
	ocrApp(QWidget *parent = 0);
	~ocrApp();


private slots:
	bool loadBt();
	bool showBt();

private:

	Ui::MainWindow ui;
	ocr_tabs* tab;
	QString filename;

};

