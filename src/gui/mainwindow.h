#pragma once

#include <QColor>
#include <QMainWindow>

struct gxt872_device;

class QComboBox;
class QLabel;
class QSlider;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void pickColor();
	void applyAll();
	void turnOff();
	void slidersChanged();

private:
	void setColor(const QColor &color);
	void setStatus(const QString &text, bool error = false);
	bool eventFilter(QObject *obj, QEvent *event) override;

	gxt872_device *dev_ = nullptr;
	QColor color_ = QColor(255, 0, 0);
	QComboBox *effect_ = nullptr;
	QLabel *preview_ = nullptr;
	QLabel *status_ = nullptr;
	QLabel *hex_ = nullptr;
	QSlider *red_ = nullptr;
	QSlider *green_ = nullptr;
	QSlider *blue_ = nullptr;
	QSlider *brightness_ = nullptr;
	QSlider *speed_ = nullptr;
};
