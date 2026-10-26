#include <QApplication>
#include <QPalette>
#include "AppState.h"
#include "MainWindow.h"
#include "SetupWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");   // tampilan konsisten di semua platform

    // --- Paksa Light Mode (abaikan tema OS dark) ---
    QPalette light;
    light.setColor(QPalette::Window,          QColor(0xF0F0F0));
    light.setColor(QPalette::WindowText,      Qt::black);
    light.setColor(QPalette::Base,            Qt::white);
    light.setColor(QPalette::AlternateBase,   QColor(0xE8E8E8));
    light.setColor(QPalette::ToolTipBase,     Qt::white);
    light.setColor(QPalette::ToolTipText,     Qt::black);
    light.setColor(QPalette::Text,            Qt::black);
    light.setColor(QPalette::Button,          QColor(0xE0E0E0));
    light.setColor(QPalette::ButtonText,      Qt::black);
    light.setColor(QPalette::BrightText,      Qt::red);
    light.setColor(QPalette::Link,            QColor(0x0057AE));
    light.setColor(QPalette::Highlight,       QColor(0x308CC6));
    light.setColor(QPalette::HighlightedText, Qt::white);
    light.setColor(QPalette::Mid,             QColor(0xB0B0B0));
    light.setColor(QPalette::Midlight,        QColor(0xD4D4D4));
    light.setColor(QPalette::Dark,            QColor(0x9A9A9A));
    light.setColor(QPalette::Shadow,          QColor(0x696969));
    a.setPalette(light);
    a.setStyleSheet(
        "QPushButton{background:palette(button);border:1px solid palette(mid);"
        "border-radius:4px;padding:6px 10px;}"
        "QPushButton:hover{background:palette(midlight);}"
        "QPushButton:pressed{background:palette(mid);}"
        "QToolButton{background:palette(button);border:1px solid palette(mid);"
        "border-radius:4px;padding:3px 8px;}"
        "QToolButton:hover{background:palette(midlight);}");

    // SetupWindow dibuat di heap agar bisa di-show/hide berkali-kali
    auto *setup = new SetupWindow;
    MainWindow *main = nullptr;

    // Setup → MainWindow
    QObject::connect(setup, &SetupWindow::continueRequested, [&] {
        if (!main) {
            main = new MainWindow;
            main->setAttribute(Qt::WA_DeleteOnClose, false);  // jangan hapus, bisa dibuka lagi

            // MainWindow → Setup (tombol "Setup")
            QObject::connect(main, &MainWindow::setupRequested, [&] {
                main->hide();
                setup->show();
                setup->raise();
                setup->activateWindow();
            });
        }
        main->show();
        setup->close();
    });

    setup->show();
    int rc = a.exec();

    AppState::instance().monitor.stop();

    // Bersihkan
    delete main;
    delete setup;
    return rc;
}
