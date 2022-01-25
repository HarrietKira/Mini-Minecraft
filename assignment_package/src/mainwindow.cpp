#include "mainwindow.h"
#include <ui_mainwindow.h>
#include "cameracontrolshelp.h"
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), cHelp()
{
    ui->setupUi(this);
    ui->mygl->setFocus();
    this->playerInfoWindow.show();
    playerInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(this->width() * 0.75, 0));

    this->inventoryWindow.ui_main = ui;
    inventoryWindow.move(QPoint(this->width()*0.5-inventoryWindow.width() * 0.5,
                                this->height()-inventoryWindow.height()));

    connect(ui->mygl, SIGNAL(sig_sendPlayerPos(QString)), &playerInfoWindow, SLOT(slot_setPosText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerVel(QString)), &playerInfoWindow, SLOT(slot_setVelText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerAcc(QString)), &playerInfoWindow, SLOT(slot_setAccText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerLook(QString)), &playerInfoWindow, SLOT(slot_setLookText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerChunk(QString)), &playerInfoWindow, SLOT(slot_setChunkText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerTerrainZone(QString)), &playerInfoWindow, SLOT(slot_setZoneText(QString)));

    connect(ui->mygl, SIGNAL(sig_inventoryWindow(bool)), this, SLOT(slot_inventoryWindow(bool)));
    connect(ui->mygl, SIGNAL(sig_updateInventory(BlockType, int)), &inventoryWindow, SLOT(slot_updateInventory(BlockType, int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_actionCamera_Controls_triggered()
{
    cHelp.show();
}
void MainWindow::slot_inventoryWindow(bool m_inventoryOpened)
{
    if(m_inventoryOpened)
    {
        this->inventoryWindow.show();
        BlockType curr = ui->mygl->m_selectedBlockType;
        if(curr == GRASS)
        {
            this->inventoryWindow.slot_activateGrass();
        }
        else if(curr == DIRT)
        {
            this->inventoryWindow.slot_activateDirt();
        }
        else if(curr == STONE)
        {
            this->inventoryWindow.slot_activateStone();
        }
        else if(curr == WATER)
        {
            this->inventoryWindow.slot_activateWater();
        }
        else if(curr == SNOW)
        {
            this->inventoryWindow.slot_activateSnow();
        }
        else if(curr == LAVA)
        {
            this->inventoryWindow.slot_activateLava();
        }
        else if(curr == BEDROCK)
        {
            this->inventoryWindow.slot_activateBedrock();
        }
        else if(curr == WOOD)
        {
            this->inventoryWindow.slot_activateWood();
        }
        else if(curr == LEAF)
        {
            this->inventoryWindow.slot_activateLeaf();
        }
    }
    else
    {
        this->inventoryWindow.close();
    }
}
