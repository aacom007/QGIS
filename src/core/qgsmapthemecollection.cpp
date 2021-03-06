/***************************************************************************
  qgsmapthemecollection.cpp
  --------------------------------------
  Date                 : September 2014
  Copyright            : (C) 2014 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmapthemecollection.h"

#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgslayertreemodellegendnode.h"
#include "qgsmaplayerlistutils.h"
#include "qgsmaplayerstylemanager.h"
#include "qgsproject.h"
#include "qgsrenderer.h"
#include "qgsvectorlayer.h"

#include <QInputDialog>

QgsMapThemeCollection::QgsMapThemeCollection( QgsProject* project )
    : mProject( project )
{
  connect( project, &QgsProject::layersRemoved, this, &QgsMapThemeCollection::registryLayersRemoved );
}

QgsMapThemeCollection::MapThemeLayerRecord QgsMapThemeCollection::createThemeLayerRecord( QgsLayerTreeLayer* nodeLayer, QgsLayerTreeModel* model )
{
  MapThemeLayerRecord layerRec( nodeLayer->layer() );
  layerRec.usingCurrentStyle = true;
  layerRec.currentStyle = nodeLayer->layer()->styleManager()->currentStyle();

  // get checked legend items
  bool hasCheckableItems = false;
  bool someItemsUnchecked = false;
  QSet<QString> checkedItems;
  Q_FOREACH ( QgsLayerTreeModelLegendNode* legendNode, model->layerLegendNodes( nodeLayer, true ) )
  {
    if ( legendNode->flags() & Qt::ItemIsUserCheckable )
    {
      hasCheckableItems = true;

      if ( legendNode->data( Qt::CheckStateRole ).toInt() == Qt::Checked )
        checkedItems << legendNode->data( QgsLayerTreeModelLegendNode::RuleKeyRole ).toString();
      else
        someItemsUnchecked = true;
    }
  }

  if ( hasCheckableItems && someItemsUnchecked )
  {
    layerRec.usingLegendItems = true;
    layerRec.checkedLegendItems = checkedItems;
  }
  return layerRec;
}

void QgsMapThemeCollection::createThemeFromCurrentState( QgsLayerTreeGroup* parent, QgsLayerTreeModel* model, QgsMapThemeCollection::MapThemeRecord& rec )
{
  Q_FOREACH ( QgsLayerTreeNode* node, parent->children() )
  {
    if ( QgsLayerTree::isGroup( node ) )
      createThemeFromCurrentState( QgsLayerTree::toGroup( node ), model, rec );
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer* nodeLayer = QgsLayerTree::toLayer( node );
      if ( nodeLayer->isVisible() )
        rec.mLayerRecords << createThemeLayerRecord( nodeLayer, model );
    }
  }
}

QgsMapThemeCollection::MapThemeRecord QgsMapThemeCollection::createThemeFromCurrentState( QgsLayerTreeGroup* root, QgsLayerTreeModel* model )
{
  QgsMapThemeCollection::MapThemeRecord rec;
  createThemeFromCurrentState( root, model, rec );
  return rec;
}

bool QgsMapThemeCollection::findRecordForLayer( QgsMapLayer* layer, const QgsMapThemeCollection::MapThemeRecord& rec, QgsMapThemeCollection::MapThemeLayerRecord& layerRec )
{
  Q_FOREACH ( const QgsMapThemeCollection::MapThemeLayerRecord& lr, rec.mLayerRecords )
  {
    if ( lr.layer() == layer )
    {
      layerRec = lr;
      return true;
    }
  }
  return false;
}

void QgsMapThemeCollection::applyThemeToLayer( QgsLayerTreeLayer* nodeLayer, QgsLayerTreeModel* model, const QgsMapThemeCollection::MapThemeRecord& rec )
{
  MapThemeLayerRecord layerRec;
  bool isVisible = findRecordForLayer( nodeLayer->layer(), rec, layerRec );

  nodeLayer->setItemVisibilityChecked( isVisible );

  if ( !isVisible )
    return;

  if ( layerRec.usingCurrentStyle )
  {
    // apply desired style first
    nodeLayer->layer()->styleManager()->setCurrentStyle( layerRec.currentStyle );
  }

  if ( layerRec.usingLegendItems )
  {
    // some nodes are not checked
    Q_FOREACH ( QgsLayerTreeModelLegendNode* legendNode, model->layerLegendNodes( nodeLayer, true ) )
    {
      QString ruleKey = legendNode->data( QgsLayerTreeModelLegendNode::RuleKeyRole ).toString();
      Qt::CheckState shouldHaveState = layerRec.checkedLegendItems.contains( ruleKey ) ? Qt::Checked : Qt::Unchecked;
      if (( legendNode->flags() & Qt::ItemIsUserCheckable ) &&
          legendNode->data( Qt::CheckStateRole ).toInt() != shouldHaveState )
        legendNode->setData( shouldHaveState, Qt::CheckStateRole );
    }
  }
  else
  {
    // all nodes should be checked
    Q_FOREACH ( QgsLayerTreeModelLegendNode* legendNode, model->layerLegendNodes( nodeLayer, true ) )
    {
      if (( legendNode->flags() & Qt::ItemIsUserCheckable ) &&
          legendNode->data( Qt::CheckStateRole ).toInt() != Qt::Checked )
        legendNode->setData( Qt::Checked, Qt::CheckStateRole );
    }
  }
}


void QgsMapThemeCollection::applyThemeToGroup( QgsLayerTreeGroup* parent, QgsLayerTreeModel* model, const QgsMapThemeCollection::MapThemeRecord& rec )
{
  Q_FOREACH ( QgsLayerTreeNode* node, parent->children() )
  {
    if ( QgsLayerTree::isGroup( node ) )
      applyThemeToGroup( QgsLayerTree::toGroup( node ), model, rec );
    else if ( QgsLayerTree::isLayer( node ) )
      applyThemeToLayer( QgsLayerTree::toLayer( node ), model, rec );
  }
}


void QgsMapThemeCollection::applyTheme( const QString& name, QgsLayerTreeGroup* root, QgsLayerTreeModel* model )
{
  applyThemeToGroup( root, model, mapThemeState( name ) );

  // also make sure that the preset is up-to-date (not containing any non-existent legend items)
  update( name, createThemeFromCurrentState( root, model ) );
}

QgsProject* QgsMapThemeCollection::project()
{
  return mProject;
}

void QgsMapThemeCollection::setProject( QgsProject* project )
{
  if ( project == mProject )
    return;

  disconnect( mProject, &QgsProject::layersRemoved, this, &QgsMapThemeCollection::registryLayersRemoved );
  mProject = project;
  connect( mProject, &QgsProject::layersRemoved, this, &QgsMapThemeCollection::registryLayersRemoved );
  emit projectChanged();
}


bool QgsMapThemeCollection::hasMapTheme( const QString& name ) const
{
  return mMapThemes.contains( name );
}

void QgsMapThemeCollection::insert( const QString& name, const QgsMapThemeCollection::MapThemeRecord& state )
{
  mMapThemes.insert( name, state );

  reconnectToLayersStyleManager();
  emit mapThemesChanged();
}

void QgsMapThemeCollection::update( const QString& name, const MapThemeRecord& state )
{
  if ( !mMapThemes.contains( name ) )
    return;

  mMapThemes[name] = state;

  reconnectToLayersStyleManager();
  emit mapThemesChanged();
}

void QgsMapThemeCollection::removeMapTheme( const QString& name )
{
  if ( !mMapThemes.contains( name ) )
    return;

  mMapThemes.remove( name );

  reconnectToLayersStyleManager();
  emit mapThemesChanged();
}

void QgsMapThemeCollection::clear()
{
  mMapThemes.clear();

  reconnectToLayersStyleManager();
  emit mapThemesChanged();
}

QStringList QgsMapThemeCollection::mapThemes() const
{
  return mMapThemes.keys();
}

QStringList QgsMapThemeCollection::mapThemeVisibleLayerIds( const QString& name ) const
{
  QStringList layerIds;
  Q_FOREACH ( const MapThemeLayerRecord& layerRec, mMapThemes.value( name ).mLayerRecords )
  {
    if ( layerRec.layer() )
      layerIds << layerRec.layer()->id();
  }
  return layerIds;
}


QList<QgsMapLayer*> QgsMapThemeCollection::mapThemeVisibleLayers( const QString& name ) const
{
  QList<QgsMapLayer*> layers;
  Q_FOREACH ( const MapThemeLayerRecord& layerRec, mMapThemes.value( name ).mLayerRecords )
  {
    if ( layerRec.layer() )
      layers << layerRec.layer();
  }
  return layers;
}


void QgsMapThemeCollection::applyMapThemeCheckedLegendNodesToLayer( const MapThemeLayerRecord& layerRec, QgsMapLayer* layer )
{
  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>( layer );
  if ( !vlayer || !vlayer->renderer() )
    return;

  QgsFeatureRenderer* renderer = vlayer->renderer();
  if ( !renderer->legendSymbolItemsCheckable() )
    return; // no need to do anything

  bool someNodesUnchecked = layerRec.usingLegendItems;

  Q_FOREACH ( const QgsLegendSymbolItem& item, vlayer->renderer()->legendSymbolItemsV2() )
  {
    bool checked = renderer->legendSymbolItemChecked( item.ruleKey() );
    bool shouldBeChecked = someNodesUnchecked ? layerRec.checkedLegendItems.contains( item.ruleKey() ) : true;
    if ( checked != shouldBeChecked )
      renderer->checkLegendSymbolItem( item.ruleKey(), shouldBeChecked );
  }
}


QMap<QString, QString> QgsMapThemeCollection::mapThemeStyleOverrides( const QString& presetName )
{
  QMap<QString, QString> styleOverrides;
  if ( !mMapThemes.contains( presetName ) )
    return styleOverrides;

  Q_FOREACH ( const MapThemeLayerRecord& layerRec, mMapThemes.value( presetName ).mLayerRecords )
  {
    if ( !layerRec.layer() )
      continue;

    if ( layerRec.usingCurrentStyle )
    {
      QgsMapLayer* layer = layerRec.layer();
      layer->styleManager()->setOverrideStyle( layerRec.currentStyle );

      // set the checked legend nodes
      applyMapThemeCheckedLegendNodesToLayer( layerRec, layer );

      // save to overrides
      QgsMapLayerStyle layerStyle;
      layerStyle.readFromLayer( layer );
      styleOverrides[layer->id()] = layerStyle.xmlData();

      layer->styleManager()->restoreOverrideStyle();
    }
  }
  return styleOverrides;
}

void QgsMapThemeCollection::reconnectToLayersStyleManager()
{
  // disconnect( 0, 0, this, SLOT( layerStyleRenamed( QString, QString ) ) );

  QSet<QgsMapLayer*> layers;
  Q_FOREACH ( const MapThemeRecord& rec, mMapThemes )
  {
    Q_FOREACH ( const MapThemeLayerRecord& layerRec, rec.mLayerRecords )
    {
      if ( layerRec.layer() )
        layers << layerRec.layer();
    }
  }

  Q_FOREACH ( QgsMapLayer* ml, layers )
  {
    connect( ml->styleManager(), SIGNAL( styleRenamed( QString, QString ) ), this, SLOT( layerStyleRenamed( QString, QString ) ) );
  }
}

void QgsMapThemeCollection::readXml( const QDomDocument& doc )
{
  clear();

  QDomElement visPresetsElem = doc.firstChildElement( QStringLiteral( "qgis" ) ).firstChildElement( QStringLiteral( "visibility-presets" ) );
  if ( visPresetsElem.isNull() )
    return;

  QDomElement visPresetElem = visPresetsElem.firstChildElement( QStringLiteral( "visibility-preset" ) );
  while ( !visPresetElem.isNull() )
  {
    QHash<QString, MapThemeLayerRecord> layerRecords; // key = layer ID

    QString presetName = visPresetElem.attribute( QStringLiteral( "name" ) );
    QDomElement visPresetLayerElem = visPresetElem.firstChildElement( QStringLiteral( "layer" ) );
    while ( !visPresetLayerElem.isNull() )
    {
      QString layerID = visPresetLayerElem.attribute( QStringLiteral( "id" ) );
      if ( QgsMapLayer* layer = mProject->mapLayer( layerID ) )
      {
        layerRecords[layerID] = MapThemeLayerRecord( layer );

        if ( visPresetLayerElem.hasAttribute( QStringLiteral( "style" ) ) )
        {
          layerRecords[layerID].usingCurrentStyle = true;
          layerRecords[layerID].currentStyle = visPresetLayerElem.attribute( QStringLiteral( "style" ) );
        }
      }
      visPresetLayerElem = visPresetLayerElem.nextSiblingElement( QStringLiteral( "layer" ) );
    }

    QDomElement checkedLegendNodesElem = visPresetElem.firstChildElement( QStringLiteral( "checked-legend-nodes" ) );
    while ( !checkedLegendNodesElem.isNull() )
    {
      QSet<QString> checkedLegendNodes;

      QDomElement checkedLegendNodeElem = checkedLegendNodesElem.firstChildElement( QStringLiteral( "checked-legend-node" ) );
      while ( !checkedLegendNodeElem.isNull() )
      {
        checkedLegendNodes << checkedLegendNodeElem.attribute( QStringLiteral( "id" ) );
        checkedLegendNodeElem = checkedLegendNodeElem.nextSiblingElement( QStringLiteral( "checked-legend-node" ) );
      }

      QString layerID = checkedLegendNodesElem.attribute( QStringLiteral( "id" ) );
      if ( mProject->mapLayer( layerID ) ) // only use valid IDs
      {
        layerRecords[layerID].usingLegendItems = true;
        layerRecords[layerID].checkedLegendItems = checkedLegendNodes;
      }
      checkedLegendNodesElem = checkedLegendNodesElem.nextSiblingElement( QStringLiteral( "checked-legend-nodes" ) );
    }

    MapThemeRecord rec;
    rec.setLayerRecords( layerRecords.values() );
    mMapThemes.insert( presetName, rec );

    visPresetElem = visPresetElem.nextSiblingElement( QStringLiteral( "visibility-preset" ) );
  }

  reconnectToLayersStyleManager();
  emit mapThemesChanged();
}

void QgsMapThemeCollection::writeXml( QDomDocument& doc )
{
  QDomElement visPresetsElem = doc.createElement( QStringLiteral( "visibility-presets" ) );
  MapThemeRecordMap::const_iterator it = mMapThemes.constBegin();
  for ( ; it != mMapThemes.constEnd(); ++ it )
  {
    QString grpName = it.key();
    const MapThemeRecord& rec = it.value();
    QDomElement visPresetElem = doc.createElement( QStringLiteral( "visibility-preset" ) );
    visPresetElem.setAttribute( QStringLiteral( "name" ), grpName );
    Q_FOREACH ( const MapThemeLayerRecord& layerRec, rec.mLayerRecords )
    {
      if ( !layerRec.layer() )
        continue;
      QString layerID = layerRec.layer()->id();
      QDomElement layerElem = doc.createElement( QStringLiteral( "layer" ) );
      layerElem.setAttribute( QStringLiteral( "id" ), layerID );
      if ( layerRec.usingCurrentStyle )
        layerElem.setAttribute( QStringLiteral( "style" ), layerRec.currentStyle );
      visPresetElem.appendChild( layerElem );

      if ( layerRec.usingLegendItems )
      {
        QDomElement checkedLegendNodesElem = doc.createElement( QStringLiteral( "checked-legend-nodes" ) );
        checkedLegendNodesElem.setAttribute( QStringLiteral( "id" ), layerID );
        Q_FOREACH ( const QString& checkedLegendNode, layerRec.checkedLegendItems )
        {
          QDomElement checkedLegendNodeElem = doc.createElement( QStringLiteral( "checked-legend-node" ) );
          checkedLegendNodeElem.setAttribute( QStringLiteral( "id" ), checkedLegendNode );
          checkedLegendNodesElem.appendChild( checkedLegendNodeElem );
        }
        visPresetElem.appendChild( checkedLegendNodesElem );
      }
    }

    visPresetsElem.appendChild( visPresetElem );
  }

  doc.firstChildElement( QStringLiteral( "qgis" ) ).appendChild( visPresetsElem );
}

void QgsMapThemeCollection::registryLayersRemoved( const QStringList& layerIDs )
{
  // TODO: this should not be necessary - layers are stored as weak pointers

  MapThemeRecordMap::iterator it = mMapThemes.begin();
  for ( ; it != mMapThemes.end(); ++it )
  {
    MapThemeRecord& rec = it.value();
    for ( int i = 0; i < rec.mLayerRecords.count(); ++i )
    {
      MapThemeLayerRecord& layerRec = rec.mLayerRecords[i];
      if ( layerRec.layer() && layerIDs.contains( layerRec.layer()->id() ) )
        rec.mLayerRecords.removeAt( i-- );
    }
  }
  emit mapThemesChanged();
}

void QgsMapThemeCollection::layerStyleRenamed( const QString& oldName, const QString& newName )
{
  QgsMapLayerStyleManager* styleMgr = qobject_cast<QgsMapLayerStyleManager*>( sender() );
  if ( !styleMgr )
    return;

  MapThemeRecordMap::iterator it = mMapThemes.begin();
  for ( ; it != mMapThemes.end(); ++it )
  {
    MapThemeRecord& rec = it.value();
    for ( int i = 0; i < rec.mLayerRecords.count(); ++i )
    {
      MapThemeLayerRecord& layerRec = rec.mLayerRecords[i];
      if ( layerRec.layer() == styleMgr->layer() )
      {
        if ( layerRec.currentStyle == oldName )
          layerRec.currentStyle = newName;
      }
    }
  }
  emit mapThemesChanged();
}

QHash<QgsMapLayer*, QgsMapThemeCollection::MapThemeLayerRecord> QgsMapThemeCollection::MapThemeRecord::validLayerRecords() const
{
  QHash<QgsMapLayer*, MapThemeLayerRecord> validSet;
  Q_FOREACH ( const MapThemeLayerRecord& layerRec, mLayerRecords )
  {
    if ( layerRec.layer() )
      validSet.insert( layerRec.layer(), layerRec );
  }
  return validSet;
}
