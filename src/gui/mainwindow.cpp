#include "mainwindow.h"

#include <QColorDialog>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "gxt872.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle(tr("GXT 872 RGB"));

	dev_ = gxt872_open(nullptr);

	auto *central = new QWidget(this);
	auto *layout = new QVBoxLayout(central);

	auto *device = new QLabel(central);
	if (dev_) {
		device->setText(tr("Tastiera: %1").arg(gxt872_path(dev_)));
	} else {
		device->setText(tr("Tastiera non trovata"));
		device->setStyleSheet("color: #c0392b;");
	}
	layout->addWidget(device);

	preview_ = new QLabel(central);
	preview_->setMinimumHeight(90);
	preview_->setAlignment(Qt::AlignCenter);
	preview_->setCursor(Qt::PointingHandCursor);
	preview_->setToolTip(tr("Clic per scegliere il colore"));
	layout->addWidget(preview_);

	auto *hex_row = new QHBoxLayout;
	hex_ = new QLabel(central);
	hex_->setObjectName("hex");
	hex_row->addWidget(hex_);
	hex_row->addStretch();
	layout->addLayout(hex_row);

	auto *grid = new QGridLayout;
	red_ = new QSlider(Qt::Horizontal, central);
	green_ = new QSlider(Qt::Horizontal, central);
	blue_ = new QSlider(Qt::Horizontal, central);
	for (QSlider *s : { red_, green_, blue_ })
		s->setRange(0, 255);
	grid->addWidget(new QLabel("R", central), 0, 0);
	grid->addWidget(red_, 0, 1);
	grid->addWidget(new QLabel("G", central), 1, 0);
	grid->addWidget(green_, 1, 1);
	grid->addWidget(new QLabel("B", central), 2, 0);
	grid->addWidget(blue_, 2, 1);
	layout->addLayout(grid);

	auto *buttons = new QHBoxLayout;
	auto *apply = new QPushButton(tr("Applica"), central);
	auto *off = new QPushButton(tr("Spegni"), central);
	buttons->addWidget(apply);
	buttons->addWidget(off);
	buttons->addStretch();
	layout->addLayout(buttons);

	status_ = new QLabel(central);
	layout->addWidget(status_);

	setCentralWidget(central);
	resize(420, 380);

	connect(apply, &QPushButton::clicked, this, &MainWindow::applyColor);
	connect(off, &QPushButton::clicked, this, &MainWindow::turnOff);
	connect(red_, &QSlider::valueChanged, this, &MainWindow::slidersChanged);
	connect(green_, &QSlider::valueChanged, this, &MainWindow::slidersChanged);
	connect(blue_, &QSlider::valueChanged, this, &MainWindow::slidersChanged);

	// Clicking the preview opens the colour dialog.
	preview_->installEventFilter(this);

	setColor(color_);
}

MainWindow::~MainWindow()
{
	gxt872_close(dev_);
}

void MainWindow::setColor(const QColor &color)
{
	color_ = color;
	red_->blockSignals(true);
	green_->blockSignals(true);
	blue_->blockSignals(true);
	red_->setValue(color.red());
	green_->setValue(color.green());
	blue_->setValue(color.blue());
	red_->blockSignals(false);
	green_->blockSignals(false);
	blue_->blockSignals(false);

	preview_->setStyleSheet(QString("background: %1; border-radius: 8px;")
					.arg(color.name()));
	hex_->setText(color.name().toUpper());
}

void MainWindow::pickColor()
{
	QColor chosen = QColorDialog::getColor(color_, this,
					      tr("Colore tastiera"));
	if (chosen.isValid())
		setColor(chosen);
}

void MainWindow::slidersChanged()
{
	setColor(QColor(red_->value(), green_->value(), blue_->value()));
}

void MainWindow::applyColor()
{
	if (!dev_) {
		setStatus(tr("Tastiera non collegata"), true);
		return;
	}
	if (gxt872_set_color(dev_, (uint8_t)color_.red(),
			     (uint8_t)color_.green(),
			     (uint8_t)color_.blue()) == 0)
		setStatus(tr("Colore applicato"));
	else
		setStatus(tr("Errore nell'invio del comando"), true);
}

void MainWindow::turnOff()
{
	if (!dev_) {
		setStatus(tr("Tastiera non collegata"), true);
		return;
	}
	if (gxt872_off(dev_) == 0)
		setStatus(tr("Retroilluminazione spenta"));
	else
		setStatus(tr("Errore nell'invio del comando"), true);
}

void MainWindow::setStatus(const QString &text, bool error)
{
	status_->setText(text);
	status_->setStyleSheet(error ? "color: #c0392b;" : "color: #27ae60;");
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == preview_ && event->type() == QEvent::MouseButtonRelease) {
		pickColor();
		return true;
	}
	return QMainWindow::eventFilter(obj, event);
}
