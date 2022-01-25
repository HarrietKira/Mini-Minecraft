#ifndef INVENTORY_H
#define INVENTORY_H

#include <QWidget>
#include <QPushButton>
#include <ui_mainwindow.h>

namespace Ui
{
    class Form;
}

class Form: public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    ~Form();

    void keyPressEvent(QKeyEvent *e);
    Ui::MainWindow *ui_main;
    int m_grassNum, m_dirtNum, m_stoneNum, m_waterNum, m_snowNum, m_lavaNum, m_bedrockNum, m_woodNum, m_leafNum;

    void slot_activateGrass();
    void slot_activateDirt();
    void slot_activateStone();
    void slot_activateWater();
    void slot_activateSnow();
    void slot_activateLava();
    void slot_activateBedrock();
    void slot_activateWood();
    void slot_activateLeaf();

    void slot_updateGrass(int num);
    void slot_updateDirt(int num);
    void slot_updateStone(int num);
    void slot_updateWater(int num);
    void slot_updateSnow(int num);
    void slot_updateLava(int num);
    void slot_updateBedrock(int num);
    void slot_updateWood(int num);
    void slot_updateLeaf(int num);
private slots:
    void on_grass_radioButton_clicked();
    void on_dirt_radioButton_clicked();
    void on_stone_radioButton_clicked();
    void on_water_radioButton_clicked();
    void on_snow_radioButton_clicked();
    void on_lava_radioButton_clicked();
    void on_bedrock_radioButton_clicked();
    void on_wood_radioButton_clicked();
    void on_leaf_radioButton_clicked();
    void slot_updateInventory(BlockType blockType, int num);
private:
    Ui::Form *ui;
};

#endif // INVENTORY_H
