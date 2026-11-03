#include <QApplication>

#include "mainwindow.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	QApplication::setApplicationName("gxt872-rgb");
	QApplication::setApplicationDisplayName("GXT 872 RGB");

	MainWindow window;
	window.show();

	return app.exec();
}
