#include <QKeyEvent>
#include <QPixmap>
#include "inventory.h"
#include "ui_inventory.h"

Form::Form(QWidget *parent) :
    QWidget(parent),
    m_grassNum(10), m_dirtNum(10), m_stoneNum(10),
    m_waterNum(10), m_snowNum(10), m_lavaNum(10),
    m_bedrockNum(10), m_woodNum(10), m_leafNum(10),
    ui(new Ui::Form)
{
    ui->setupUi(this);
}

Form::~Form()
{
    delete ui;
}

void Form::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_I)
    {
        this->close();
    }
}

void Form::on_grass_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = GRASS;
}
void Form::on_dirt_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = DIRT;
}
void Form::on_stone_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = STONE;
}
void Form::on_water_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = WATER;
}
void Form::on_snow_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = SNOW;
}
void Form::on_lava_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = LAVA;
}
void Form::on_bedrock_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = BEDROCK;
}
void Form::on_wood_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = WOOD;
}
void Form::on_leaf_radioButton_clicked()
{
    ui_main->mygl->m_selectedBlockType = LEAF;
}
void Form::slot_updateGrass(int num)
{
    m_grassNum = num;
    ui->grass_LCDnum->display(num);
    if(num <= 0) ui->grass_radioButton->setCheckable(false);
    else ui->grass_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_GRASSPlacable = false;
    else ui_main->mygl->m_GRASSPlacable = true;
}
void Form::slot_updateDirt(int num)
{
    m_dirtNum = num;
    ui->dirt_LCDnum->display(num);
    if(num <= 0) ui->dirt_radioButton->setCheckable(false);
    else ui->dirt_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_DIRTPlaceable = false;
    else ui_main->mygl->m_DIRTPlaceable = true;
}
void Form::slot_updateStone(int num)
{
    m_stoneNum = num;
    ui->stone_LCDnum->display(num);
    if(num <= 0) ui->stone_radioButton->setCheckable(false);
    else ui->stone_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_STONEPlacable = false;
    else ui_main->mygl->m_STONEPlacable = true;
}
void Form::slot_updateWater(int num)
{
    m_waterNum = num;
    ui->water_LCDnum->display(num);
    if(num <= 0) ui->water_radioButton->setCheckable(false);
    else ui->water_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_WATERPlacable = false;
    else ui_main->mygl->m_WATERPlacable = true;
}
void Form::slot_updateSnow(int num)
{
    m_snowNum = num;
    ui->snow_LCDnum->display(num);
    if(num <= 0) ui->snow_radioButton->setCheckable(false);
    else ui->snow_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_SNOWPlacable = false;
    else ui_main->mygl->m_SNOWPlacable = true;
}
void Form::slot_updateLava(int num)
{
    m_lavaNum = num;
    ui->lava_LCDnum->display(num);
    if(num <= 0) ui->lava_radioButton->setCheckable(false);
    else ui->lava_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_LAVAPlacable = false;
    else ui_main->mygl->m_LAVAPlacable = true;
}
void Form::slot_updateBedrock(int num)
{
    m_bedrockNum = num;
    ui->bedrock_LCDnum->display(num);
    if(num <= 0) ui->bedrock_radioButton->setCheckable(false);
    else ui->bedrock_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_BEDROCKPlacable = false;
    else ui_main->mygl->m_BEDROCKPlacable = true;
}
void Form::slot_updateWood(int num)
{
    m_woodNum = num;
    ui->wood_LCDnum->display(num);
    if(num <= 0) ui->wood_radioButton->setCheckable(false);
    else ui->wood_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_WOODPlacable = false;
    else ui_main->mygl->m_WOODPlacable = true;
}
void Form::slot_updateLeaf(int num)
{
    m_leafNum = num;
    ui->leaf_LCDnum->display(num);
    if(num <= 0) ui->leaf_radioButton->setCheckable(false);
    else ui->leaf_radioButton->setCheckable(true);
    if(num <= 0) ui_main->mygl->m_LEAFPlacable = false;
    else ui_main->mygl->m_LEAFPlacable = true;
}

void Form::slot_activateGrass()
{
    ui->grass_radioButton->setChecked(true);
}
void Form::slot_activateDirt()
{
    ui->dirt_radioButton->setChecked(true);
}
void Form::slot_activateStone()
{
    ui->stone_radioButton->setChecked(true);
}
void Form::slot_activateWater()
{
    ui->water_radioButton->setChecked(true);
}
void Form::slot_activateSnow()
{
    ui->snow_radioButton->setChecked(true);
}
void Form::slot_activateLava()
{
    ui->lava_radioButton->setChecked(true);
}
void Form::slot_activateBedrock()
{
    ui->bedrock_radioButton->setChecked(true);
}
void Form::slot_activateWood()
{
    ui->wood_radioButton->setChecked(true);
}
void Form::slot_activateLeaf()
{
    ui->leaf_radioButton->setChecked(true);
}
void Form::slot_updateInventory(BlockType blockType, int num)
{
    if(blockType == GRASS)
    {
        slot_updateGrass(m_grassNum + num);
    }
    else if(blockType == DIRT)
    {
        slot_updateDirt(m_dirtNum + num);
    }
    else if(blockType == STONE)
    {
        slot_updateStone(m_stoneNum + num);
    }
    else if(blockType == WATER)
    {
        slot_updateWater(m_waterNum + num);
    }
    else if(blockType == SNOW)
    {
        slot_updateSnow(m_snowNum + num);
    }
    else if(blockType == LAVA)
    {
        slot_updateLava(m_lavaNum + num);
    }
    else if(blockType == BEDROCK)
    {
        slot_updateBedrock(m_bedrockNum + num);
    }
    else if(blockType == WOOD)
    {
        slot_updateWood(m_woodNum + num);
    }
    else if(blockType == LEAF)
    {
        slot_updateLeaf(m_leafNum + num);
    }
}
