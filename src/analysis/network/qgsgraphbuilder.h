/***************************************************************************
  qgsgraphbuilder.h
  --------------------------------------
  Date                 : 2010-10-25
  Copyright            : (C) 2010 by Yakushev Sergey
  Email                : YakushevS@list.ru
****************************************************************************
*                                                                          *
*   This program is free software; you can redistribute it and/or modify   *
*   it under the terms of the GNU General Public License as published by   *
*   the Free Software Foundation; either version 2 of the License, or      *
*   (at your option) any later version.                                    *
*                                                                          *
***************************************************************************/

#ifndef QGSGRAPHBUILDER_H
#define QGSGRAPHBUILDER_H

#include "qgsgraphbuilderinterface.h"

#include <qgsspatialindex.h>
#include "qgis_analysis.h"

class QgsDistanceArea;
class QgsCoordinateTransform;
class QgsGraph;

/**
* \ingroup analysis
* \class QgsGraphBuilder
* \brief This class used for making the QgsGraph object
*/

class ANALYSIS_EXPORT QgsGraphBuilder : public QgsGraphBuilderInterface
{
  public:

    /**
     * Default constructor
     */
    QgsGraphBuilder( const QgsCoordinateReferenceSystem& crs, bool otfEnabled = true, double topologyTolerance = 0.0, const QString& ellipsoidID = "WGS84" );

    ~QgsGraphBuilder();

    /*
     * MANDATORY BUILDER PROPERTY DECLARATION
     */
    virtual void addVertex( int id, const QgsPoint& pt ) override;

    virtual void addEdge( int pt1id, const QgsPoint& pt1, int pt2id, const QgsPoint& pt2, const QVector< QVariant >& prop ) override;

    /**
     * Returns generated QgsGraph
     */
    QgsGraph* graph();

  private:

    QgsGraph *mGraph = nullptr;
};

#endif // QGSGRAPHBUILDER_H
