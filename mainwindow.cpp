#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "xeditor.h"

XEditor* createEditor(QWidget* parent);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tabWidget->addTab(createEditor(ui->tabWidget),"untitled.uvm");
}

MainWindow::~MainWindow()
{
    delete ui;
}
