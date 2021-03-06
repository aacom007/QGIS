/***************************************************************************
  testqgslayertree.cpp
  --------------------------------------
  Date                 : September 2015
  Copyright            : (C) 2015 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgstest.h"

#include <qgsapplication.h>
#include <qgslayertree.h>
#include <qgsproject.h>
#include <qgsvectorlayer.h>
#include <qgsvectorlayerdiagramprovider.h>
#include <qgsvectorlayerlabelprovider.h>
#include <qgscategorizedsymbolrenderer.h>
#include <qgsgraduatedsymbolrenderer.h>
#include <qgsrulebasedrenderer.h>
#include <qgslayertreemodel.h>
#include <qgslayertreemodellegendnode.h>

class TestQgsLayerTree : public QObject
{
    Q_OBJECT
  public:
    TestQgsLayerTree() : mRoot( 0 ) {}
  private slots:
    void initTestCase();
    void cleanupTestCase();
    void testGroupNameChanged();
    void testLayerNameChanged();
    void testCheckStateHiearchical();
    void testCheckStateMutuallyExclusive();
    void testCheckStateMutuallyExclusiveEdgeCases();
    void testShowHideAllSymbolNodes();
    void testFindLegendNode();
    void testLegendSymbolCategorized();
    void testLegendSymbolGraduated();
    void testLegendSymbolRuleBased();
    void testResolveReferences();

  private:

    QgsLayerTreeGroup* mRoot = nullptr;

    void testRendererLegend( QgsFeatureRenderer* renderer );

    bool childVisiblity( int childIndex ) const
    {
      return mRoot->children().at( childIndex )->isVisible();
    }

    bool visibilityChecked( int childIndex ) const
    {
      return mRoot->children().at( childIndex )->itemVisibilityChecked();
    }

    void setVisibilityChecked( int childIndex, bool state )
    {
      mRoot->children().at( childIndex )->setItemVisibilityChecked( state );
    }
};

void TestQgsLayerTree::initTestCase()
{
  QgsApplication::init();
  QgsApplication::initQgis();

  mRoot = new QgsLayerTreeGroup();
  mRoot->addGroup( QStringLiteral( "grp1" ) );
  mRoot->addGroup( QStringLiteral( "grp2" ) );
  mRoot->addGroup( QStringLiteral( "grp3" ) );

  // all cases start with all items checked
}

void TestQgsLayerTree::cleanupTestCase()
{
  delete mRoot;
  QgsApplication::exitQgis();
}

void TestQgsLayerTree::testGroupNameChanged()
{
  QgsLayerTreeNode* secondGroup = mRoot->children()[1];

  QSignalSpy spy( mRoot, SIGNAL( nameChanged( QgsLayerTreeNode*, QString ) ) );
  secondGroup->setName( "grp2+" );

  QCOMPARE( secondGroup->name(), QString( "grp2+" ) );

  QCOMPARE( spy.count(), 1 );
  QList<QVariant> arguments = spy.takeFirst();
  QCOMPARE( arguments.at( 0 ).value<QgsLayerTreeNode*>(), secondGroup );
  QCOMPARE( arguments.at( 1 ).toString(), QString( "grp2+" ) );

  secondGroup->setName( "grp2" );
  QCOMPARE( secondGroup->name(), QString( "grp2" ) );
}

void TestQgsLayerTree::testLayerNameChanged()
{
  QgsVectorLayer* vl = new QgsVectorLayer( QStringLiteral( "Point?field=col1:integer" ), QStringLiteral( "vl" ), QStringLiteral( "memory" ) );
  QVERIFY( vl->isValid() );

  QgsLayerTreeLayer* n = new QgsLayerTreeLayer( vl->id(), vl->name() );
  mRoot->addChildNode( n );

  QSignalSpy spy( mRoot, SIGNAL( nameChanged( QgsLayerTreeNode*, QString ) ) );

  QCOMPARE( n->name(), QString( "vl" ) );
  n->setName( "changed 1" );

  QCOMPARE( n->name(), QString( "changed 1" ) );
  QCOMPARE( spy.count(), 1 );
  QList<QVariant> arguments = spy.takeFirst();
  QCOMPARE( arguments.at( 0 ).value<QgsLayerTreeNode*>(), n );
  QCOMPARE( arguments.at( 1 ).toString(), QString( "changed 1" ) );

  QgsProject project;
  project.addMapLayer( vl );
  n->resolveReferences( &project );

  // set name via map layer
  vl->setName( "changed 2" );
  QCOMPARE( n->name(), QString( "changed 2" ) );
  QCOMPARE( spy.count(), 1 );
  arguments = spy.takeFirst();
  QCOMPARE( arguments.at( 0 ).value<QgsLayerTreeNode*>(), n );
  QCOMPARE( arguments.at( 1 ).toString(), QString( "changed 2" ) );

  // set name via layer tree
  n->setName( "changed 3" );
  QCOMPARE( n->name(), QString( "changed 3" ) );
  QCOMPARE( spy.count(), 1 );
  arguments = spy.takeFirst();
  QCOMPARE( arguments.at( 0 ).value<QgsLayerTreeNode*>(), n );
  QCOMPARE( arguments.at( 1 ).toString(), QString( "changed 3" ) );

  mRoot->removeChildNode( n );
}

void TestQgsLayerTree::testCheckStateHiearchical()
{
  mRoot->setItemVisibilityCheckedRecursive( false );
  QCOMPARE( mRoot->isItemVisibilityCheckedRecursive(), false );
  QCOMPARE( mRoot->isItemVisibilityUncheckedRecursive(), true );
  QCOMPARE( visibilityChecked( 0 ), false );
  QCOMPARE( visibilityChecked( 1 ), false );
  QCOMPARE( visibilityChecked( 2 ), false );

  mRoot->children().at( 0 )->setItemVisibilityCheckedParentRecursive( true );
  QCOMPARE( mRoot->itemVisibilityChecked(), true );

  QCOMPARE( mRoot->isItemVisibilityCheckedRecursive(), false );
  QCOMPARE( mRoot->isItemVisibilityUncheckedRecursive(), false );

  mRoot->setItemVisibilityCheckedRecursive( true );
  QCOMPARE( mRoot->isItemVisibilityCheckedRecursive(), true );
  QCOMPARE( mRoot->isItemVisibilityUncheckedRecursive(), false );
  QCOMPARE( visibilityChecked( 0 ), true );
  QCOMPARE( visibilityChecked( 1 ), true );
  QCOMPARE( visibilityChecked( 2 ), true );
}

void TestQgsLayerTree::testCheckStateMutuallyExclusive()
{
  mRoot->setIsMutuallyExclusive( true );

  // only first should be enabled
  QCOMPARE( childVisiblity( 0 ), true );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), false );
  QCOMPARE( mRoot->isVisible(), true ); // fully checked, not just partial

  // switch to some other child
  setVisibilityChecked( 2, true );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), true );
  QCOMPARE( mRoot->isVisible(), true );

  // now uncheck the root
  mRoot->setItemVisibilityChecked( false );
  QCOMPARE( mRoot->itemVisibilityChecked(), false );

  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( visibilityChecked( 2 ), true );
  QCOMPARE( childVisiblity( 2 ), false );
  QCOMPARE( mRoot->isVisible(), false );

  // check one of the children - should not modify the root
  setVisibilityChecked( 2, true );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), false );
  QCOMPARE( mRoot->itemVisibilityChecked(), false );
  QCOMPARE( mRoot->isVisible(), false );

  // uncheck the child - should not modify the root
  setVisibilityChecked( 2, false );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), false );
  QCOMPARE( mRoot->itemVisibilityChecked(), false );
  QCOMPARE( mRoot->isVisible(), false );

  // check the root back
  mRoot->setItemVisibilityChecked( true );
  setVisibilityChecked( 2, true );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), true );
  QCOMPARE( mRoot->isVisible(), true );

  // remove a child
  mRoot->removeChildNode( mRoot->children().at( 0 ) );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), true );
  QCOMPARE( mRoot->isVisible(), true );

  // add the group back - will not be checked
  mRoot->insertGroup( 0, QStringLiteral( "grp1" ) );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), true );
  QCOMPARE( mRoot->isVisible(), true );

  // remove a child that is checked
  mRoot->removeChildNode( mRoot->children().at( 2 ) );
  QCOMPARE( childVisiblity( 0 ), false );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( mRoot->isVisible(), true );

  // add the item back
  setVisibilityChecked( 0, true );
  mRoot->addGroup( QStringLiteral( "grp3" ) );
  QCOMPARE( childVisiblity( 0 ), true );
  QCOMPARE( childVisiblity( 1 ), false );
  QCOMPARE( childVisiblity( 2 ), false );
  QCOMPARE( mRoot->isVisible(), true );

  mRoot->setIsMutuallyExclusive( false );

  // go back to original state
  mRoot->setItemVisibilityChecked( true );
}

void TestQgsLayerTree::testCheckStateMutuallyExclusiveEdgeCases()
{
  // starting with empty mutually exclusive group
  QgsLayerTreeGroup* root2 = new QgsLayerTreeGroup();
  root2->setIsMutuallyExclusive( true );
  root2->addGroup( QStringLiteral( "1" ) );
  QCOMPARE( QgsLayerTree::toGroup( root2->children().at( 0 ) )->isVisible(), true );
  root2->addGroup( QStringLiteral( "2" ) );
  QCOMPARE( QgsLayerTree::toGroup( root2->children().at( 0 ) )->isVisible(), true );
  QCOMPARE( QgsLayerTree::toGroup( root2->children().at( 1 ) )->isVisible(), false );
  delete root2;

  // check-uncheck the only child
  QgsLayerTreeGroup* root3 = new QgsLayerTreeGroup();
  root3->setIsMutuallyExclusive( true );
  root3->addGroup( QStringLiteral( "1" ) );
  QCOMPARE( QgsLayerTree::toGroup( root3->children().at( 0 ) )->isVisible(), true );
  QgsLayerTree::toGroup( root3->children().at( 0 ) )->setItemVisibilityChecked( false );
  QCOMPARE( QgsLayerTree::toGroup( root3->children().at( 0 ) )->isVisible(), false );
  QCOMPARE( root3->isVisible(), true );
  QgsLayerTree::toGroup( root3->children().at( 0 ) )->setItemVisibilityChecked( true );
  QCOMPARE( QgsLayerTree::toGroup( root3->children().at( 0 ) )->isVisible(), true );
  QCOMPARE( root3->isVisible(), true );
  delete root3;
}

void TestQgsLayerTree::testShowHideAllSymbolNodes()
{
  //new memory layer
  QgsVectorLayer* vl = new QgsVectorLayer( QStringLiteral( "Point?field=col1:integer" ), QStringLiteral( "vl" ), QStringLiteral( "memory" ) );
  QVERIFY( vl->isValid() );

  QgsProject project;
  project.addMapLayer( vl );

  //create a categorized renderer for layer
  QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer();
  renderer->setClassAttribute( QStringLiteral( "col1" ) );
  renderer->setSourceSymbol( QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ) );
  renderer->addCategory( QgsRendererCategory( "a", QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ), QStringLiteral( "a" ) ) );
  renderer->addCategory( QgsRendererCategory( "b", QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ), QStringLiteral( "b" ) ) );
  renderer->addCategory( QgsRendererCategory( "c", QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ), QStringLiteral( "c" ) ) );
  vl->setRenderer( renderer );

  //create legend with symbology nodes for categorized renderer
  QgsLayerTreeGroup* root = new QgsLayerTreeGroup();
  QgsLayerTreeLayer* n = new QgsLayerTreeLayer( vl );
  root->addChildNode( n );
  QgsLayerTreeModel* m = new QgsLayerTreeModel( root, 0 );
  m->refreshLayerLegend( n );

  //test that all nodes are initially checked
  QList<QgsLayerTreeModelLegendNode*> nodes = m->layerLegendNodes( n );
  QCOMPARE( nodes.length(), 3 );
  Q_FOREACH ( QgsLayerTreeModelLegendNode* ln, nodes )
  {
    QVERIFY( ln->data( Qt::CheckStateRole ) == Qt::Checked );
  }
  //uncheck all and test that all nodes are unchecked
  static_cast< QgsSymbolLegendNode* >( nodes.at( 0 ) )->uncheckAllItems();
  Q_FOREACH ( QgsLayerTreeModelLegendNode* ln, nodes )
  {
    QVERIFY( ln->data( Qt::CheckStateRole ) == Qt::Unchecked );
  }
  //check all and test that all nodes are checked
  static_cast< QgsSymbolLegendNode* >( nodes.at( 0 ) )->checkAllItems();
  Q_FOREACH ( QgsLayerTreeModelLegendNode* ln, nodes )
  {
    QVERIFY( ln->data( Qt::CheckStateRole ) == Qt::Checked );
  }

  //cleanup
  delete m;
  delete root;
}

void TestQgsLayerTree::testFindLegendNode()
{
  //new memory layer
  QgsVectorLayer* vl = new QgsVectorLayer( QStringLiteral( "Point?field=col1:integer" ), QStringLiteral( "vl" ), QStringLiteral( "memory" ) );
  QVERIFY( vl->isValid() );

  QgsProject project;
  project.addMapLayer( vl );

  //create a categorized renderer for layer
  QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer();
  renderer->setClassAttribute( QStringLiteral( "col1" ) );
  renderer->setSourceSymbol( QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ) );
  renderer->addCategory( QgsRendererCategory( "a", QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ), QStringLiteral( "a" ) ) );
  renderer->addCategory( QgsRendererCategory( "b", QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ), QStringLiteral( "b" ) ) );
  renderer->addCategory( QgsRendererCategory( "c", QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ), QStringLiteral( "c" ) ) );
  vl->setRenderer( renderer );

  //create legend with symbology nodes for categorized renderer
  QgsLayerTreeGroup* root = new QgsLayerTreeGroup();
  QgsLayerTreeModel* m = new QgsLayerTreeModel( root, 0 );
  QVERIFY( !m->findLegendNode( QString( "id" ), QString( "rule" ) ) );
  QgsLayerTreeLayer* n = new QgsLayerTreeLayer( vl );
  root->addChildNode( n );
  m->refreshLayerLegend( n );
  QVERIFY( !m->findLegendNode( QString( "id" ), QString( "rule" ) ) );
  QVERIFY( !m->findLegendNode( QString( "vl" ), QString( "rule" ) ) );

  QgsLegendSymbolListV2 symbolList = renderer->legendSymbolItemsV2();
  Q_FOREACH ( const QgsLegendSymbolItem& symbol, symbolList )
  {
    QgsLayerTreeModelLegendNode* found = m->findLegendNode( vl->id(), symbol.ruleKey() );
    QVERIFY( found );
    QCOMPARE( found->layerNode()->layerId(), vl->id() );
    QCOMPARE( found->data( Qt::DisplayRole ).toString(), symbol.label() );
  }

  //cleanup
  delete m;
  delete root;
}

void TestQgsLayerTree::testLegendSymbolCategorized()
{
  //test retrieving/setting a categorized renderer's symbol through the legend node
  QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer();
  renderer->setClassAttribute( QStringLiteral( "col1" ) );
  renderer->setSourceSymbol( QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ) );
  QgsStringMap props;
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#ff0000" ) );
  renderer->addCategory( QgsRendererCategory( "a", QgsMarkerSymbol::createSimple( props ), QStringLiteral( "a" ) ) );
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#00ff00" ) );
  renderer->addCategory( QgsRendererCategory( "b", QgsMarkerSymbol::createSimple( props ), QStringLiteral( "b" ) ) );
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#0000ff" ) );
  renderer->addCategory( QgsRendererCategory( "c", QgsMarkerSymbol::createSimple( props ), QStringLiteral( "c" ) ) );
  testRendererLegend( renderer );
}

void TestQgsLayerTree::testLegendSymbolGraduated()
{
  //test retrieving/setting a graduated renderer's symbol through the legend node
  QgsGraduatedSymbolRenderer* renderer = new QgsGraduatedSymbolRenderer();
  renderer->setClassAttribute( QStringLiteral( "col1" ) );
  renderer->setSourceSymbol( QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry ) );
  QgsStringMap props;
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#ff0000" ) );
  renderer->addClass( QgsRendererRange( 1, 2, QgsMarkerSymbol::createSimple( props ), QStringLiteral( "a" ) ) );
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#00ff00" ) );
  renderer->addClass( QgsRendererRange( 2, 3, QgsMarkerSymbol::createSimple( props ), QStringLiteral( "b" ) ) );
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#0000ff" ) );
  renderer->addClass( QgsRendererRange( 3, 4, QgsMarkerSymbol::createSimple( props ), QStringLiteral( "c" ) ) );
  testRendererLegend( renderer );
}

void TestQgsLayerTree::testLegendSymbolRuleBased()
{
  //test retrieving/setting a rule based renderer's symbol through the legend node
  QgsRuleBasedRenderer::Rule* root = new QgsRuleBasedRenderer::Rule( 0 );
  QgsStringMap props;
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#ff0000" ) );
  root->appendChild( new QgsRuleBasedRenderer::Rule( QgsMarkerSymbol::createSimple( props ), 0, 0, QStringLiteral( "\"col1\"=1" ) ) );
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#00ff00" ) );
  root->appendChild( new QgsRuleBasedRenderer::Rule( QgsMarkerSymbol::createSimple( props ), 0, 0, QStringLiteral( "\"col1\"=2" ) ) );
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#0000ff" ) );
  root->appendChild( new QgsRuleBasedRenderer::Rule( QgsMarkerSymbol::createSimple( props ), 0, 0, QStringLiteral( "ELSE" ) ) );
  QgsRuleBasedRenderer* renderer = new QgsRuleBasedRenderer( root );
  testRendererLegend( renderer );
}

void TestQgsLayerTree::testResolveReferences()
{
  QgsVectorLayer* vl = new QgsVectorLayer( QStringLiteral( "Point?field=col1:integer" ), QStringLiteral( "vl" ), QStringLiteral( "memory" ) );
  QVERIFY( vl->isValid() );

  QString n1id = vl->id();
  QString n2id = "XYZ";

  QgsMapLayer* nullLayer = nullptr; // QCOMPARE does not like nullptr directly

  QgsLayerTreeGroup* root = new QgsLayerTreeGroup();
  QgsLayerTreeLayer* n1 = new QgsLayerTreeLayer( n1id, vl->name() );
  QgsLayerTreeLayer* n2 = new QgsLayerTreeLayer( n2id, "invalid layer" );
  root->addChildNode( n1 );
  root->addChildNode( n2 );

  // layer object not yet accessible
  QCOMPARE( n1->layer(), nullLayer );
  QCOMPARE( n1->layerId(), n1id );
  QCOMPARE( n2->layer(), nullLayer );
  QCOMPARE( n2->layerId(), n2id );

  QgsProject project;
  project.addMapLayer( vl );

  root->resolveReferences( &project );

  // now the layer should be accessible
  QCOMPARE( n1->layer(), vl );
  QCOMPARE( n1->layerId(), n1id );
  QCOMPARE( n2->layer(), nullLayer );
  QCOMPARE( n2->layerId(), n2id );

  project.removeMapLayer( vl ); // deletes the layer

  // layer object not accessible anymore
  QCOMPARE( n1->layer(), nullLayer );
  QCOMPARE( n1->layerId(), n1id );
  QCOMPARE( n2->layer(), nullLayer );
  QCOMPARE( n2->layerId(), n2id );

  delete root;
}

void TestQgsLayerTree::testRendererLegend( QgsFeatureRenderer* renderer )
{
  // runs renderer legend through a bunch of legend symbol tests

  // NOTE: test expects renderer with at least 3 symbol nodes, where the initial symbol colors should be:
  // #ff0000, #00ff00, #0000ff

  //new memory layer
  QgsVectorLayer* vl = new QgsVectorLayer( QStringLiteral( "Point?field=col1:integer" ), QStringLiteral( "vl" ), QStringLiteral( "memory" ) );
  QVERIFY( vl->isValid() );

  QgsProject project;
  project.addMapLayer( vl );

  vl->setRenderer( renderer );

  //create legend with symbology nodes for renderer
  QgsLayerTreeGroup* root = new QgsLayerTreeGroup();
  QgsLayerTreeLayer* n = new QgsLayerTreeLayer( vl );
  root->addChildNode( n );
  QgsLayerTreeModel* m = new QgsLayerTreeModel( root, 0 );
  m->refreshLayerLegend( n );

  //test initial symbol
  QgsLegendSymbolListV2 symbolList = renderer->legendSymbolItemsV2();
  Q_FOREACH ( const QgsLegendSymbolItem& symbol, symbolList )
  {
    QgsSymbolLegendNode* symbolNode = dynamic_cast< QgsSymbolLegendNode* >( m->findLegendNode( vl->id(), symbol.ruleKey() ) );
    QVERIFY( symbolNode );
    QCOMPARE( symbolNode->symbol()->color(), symbol.symbol()->color() );
  }
  //try changing a symbol's color
  QgsSymbolLegendNode* symbolNode = dynamic_cast< QgsSymbolLegendNode* >( m->findLegendNode( vl->id(), symbolList.at( 1 ).ruleKey() ) );
  QVERIFY( symbolNode );
  QgsSymbol* newSymbol = symbolNode->symbol()->clone();
  newSymbol->setColor( QColor( 255, 255, 0 ) );
  symbolNode->setSymbol( newSymbol );
  QCOMPARE( symbolNode->symbol()->color(), QColor( 255, 255, 0 ) );
  //test that symbol change was sent to renderer
  symbolList = renderer->legendSymbolItemsV2();
  QCOMPARE( symbolList.at( 1 ).symbol()->color(), QColor( 255, 255, 0 ) );

  //another test - check directly setting symbol at renderer
  QgsStringMap props;
  props.insert( QStringLiteral( "color" ), QStringLiteral( "#00ffff" ) );
  renderer->setLegendSymbolItem( symbolList.at( 2 ).ruleKey(), QgsMarkerSymbol::createSimple( props ) );
  m->refreshLayerLegend( n );
  symbolNode = dynamic_cast< QgsSymbolLegendNode* >( m->findLegendNode( vl->id(), symbolList.at( 2 ).ruleKey() ) );
  QVERIFY( symbolNode );
  QCOMPARE( symbolNode->symbol()->color(), QColor( 0, 255, 255 ) );

  //cleanup
  delete m;
  delete root;
}


QGSTEST_MAIN( TestQgsLayerTree )
#include "testqgslayertree.moc"
