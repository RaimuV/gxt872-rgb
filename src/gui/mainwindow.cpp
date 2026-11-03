#include "mainwindow.h"

#include <QColorDialog>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "gxt872.h"

namespace {

struct EffectEntry {
	const char *name;
	int value;
};

const EffectEntry kEffects[] = {
	{ QT_TR_NOOP("Statico"), GXT872_EFFECT_STATIC },
	{ QT_TR_NOOP("Respiro"), GXT872_EFFECT_BREATHING },
	{ QT_TR_NOOP("Ciclo arcobaleno"), GXT872_EFFECT_RAINBOW_CYCLE },
	{ QT_TR_NOOP("Onda orizzontale"), GXT872_EFFECT_HORIZONTAL_RIPPLE },
	{ QT_TR_NOOP("Gocce"), GXT872_EFFECT_RAINDROPS },
	{ QT_TR_NOOP("Ripple burst"), GXT872_EFFECT_RIPPLE_BURST },
	{ QT_TR_NOOP("Scintille"), GXT872_EFFECT_SPARKLE },
	{ QT_TR_NOOP("Serpente"), GXT872_EFFECT_SNAKE },
	{ QT_TR_NOOP("Onda arcobaleno"), GXT872_EFFECT_RAINBOW_WAVE },
	{ QT_TR_NOOP("Reattivo"), GXT872_EFFECT_REACTIVE },
	{ QT_TR_NOOP("Onda sinusoidale"), GXT872_EFFECT_SINE_WAVE },
	{ QT_TR_NOOP("Ruota arcobaleno"), GXT872_EFFECT_RAINBOW_WHEEL },
	{ QT_TR_NOOP("Cascata arcobaleno"), GXT872_EFFECT_RAINBOW_WATERFALL },
	{ QT_TR_NOOP("Bloom"), GXT872_EFFECT_BLOOM },
};

QSlider *makeSlider(int min, int max, int value, QWidget *parent)
{
	auto *s = new QSlider(Qt::Horizontal, parent);
	s->setRange(min, max);
	s->setValue(value);
	return s;
}

} // namespace

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

	auto *effect_row = new QHBoxLayout;
	effect_row->addWidget(new QLabel(tr("Effetto"), central));
	effect_ = new QComboBox(central);
	for (const auto &e : kEffects)
		effect_->addItem(tr(e.name), e.value);
	effect_row->addWidget(effect_, 1);
	layout->addLayout(effect_row);

	preview_ = new QLabel(central);
	preview_->setMinimumHeight(90);
	preview_->setCursor(Qt::PointingHandCursor);
	preview_->setToolTip(tr("Clic per scegliere il colore"));
	preview_->installEventFilter(this);
	layout->addWidget(preview_);

	hex_ = new QLabel(central);
	layout->addWidget(hex_);

	auto *grid = new QGridLayout;
	red_ = makeSlider(0, 255, color_.red(), central);
	green_ = makeSlider(0, 255, color_.green(), central);
	blue_ = makeSlider(0, 255, color_.blue(), central);
	grid->addWidget(new QLabel("R", central), 0, 0);
	grid->addWidget(red_, 0, 1);
	grid->addWidget(new QLabel("G", central), 1, 0);
	grid->addWidget(green_, 1, 1);
	grid->addWidget(new QLabel("B", central), 2, 0);
	grid->addWidget(blue_, 2, 1);
	layout->addLayout(grid);

	auto *bright_row = new QHBoxLayout;
	bright_row->addWidget(new QLabel(tr("Luminosità"), central));
	brightness_ = makeSlider(GXT872_BRIGHTNESS_MIN, GXT872_BRIGHTNESS_MAX, 2,
				 central);
	bright_row->addWidget(brightness_, 1);
	layout->addLayout(bright_row);

	auto *speed_row = new QHBoxLayout;
	speed_row->addWidget(new QLabel(tr("Velocità"), central));
	speed_ = makeSlider(GXT872_SPEED_MIN, GXT872_SPEED_MAX, 2, central);
	speed_row->addWidget(speed_, 1);
	layout->addLayout(speed_row);

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
	resize(460, 520);

	connect(apply, &QPushButton::clicked, this, &MainWindow::applyAll);
	connect(off, &QPushButton::clicked, this, &MainWindow::turnOff);
	connect(red_, &QSlider::valueChanged, this, &MainWindow::slidersChanged);
	connect(green_, &QSlider::valueChanged, this, &MainWindow::slidersChanged);
	connect(blue_, &QSlider::valueChanged, this, &MainWindow::slidersChanged);

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

void MainWindow::applyAll()
{
	if (!dev_) {
		setStatus(tr("Tastiera non collegata"), true);
		return;
	}
	if (gxt872_set_color(dev_, (uint8_t)color_.red(),
			     (uint8_t)color_.green(),
			     (uint8_t)color_.blue()) < 0 ||
	    gxt872_set_effect(dev_, (enum gxt872_effect)effect_
				      ->currentData().toInt()) < 0 ||
	    gxt872_set_brightness(dev_, brightness_->value()) < 0 ||
	    gxt872_set_speed(dev_, speed_->value()) < 0) {
		setStatus(tr("Errore nell'invio del comando"), true);
		return;
	}
	setStatus(tr("Impostazioni applicate"));
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
