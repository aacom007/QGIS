/***************************************************************************
                        qgsmulticurve.h
  -------------------------------------------------------------------
Date                 : 28 Oct 2014
Copyright            : (C) 2014 by Marco Hugentobler
email                : marco.hugentobler at sourcepole dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMULTICURVEV2_H
#define QGSMULTICURVEV2_H

#include "qgis_core.h"
#include "qgsgeometrycollection.h"

/** \ingroup core
 * \class QgsMultiCurve
 * \brief Multi curve geometry collection.
 * \note added in QGIS 2.10
 * \note this API is not considered stable and may change for 2.12
 */
class CORE_EXPORT QgsMultiCurve: public QgsGeometryCollection
{
  public:
    QgsMultiCurve();
    virtual QString geometryType() const override { return QStringLiteral( "MultiCurve" ); }
    QgsMultiCurve* clone() const override;

    bool fromWkt( const QString& wkt ) override;

    // inherited: int wkbSize() const;
    // inherited: unsigned char* asWkb( int& binarySize ) const;
    // inherited: QString asWkt( int precision = 17 ) const;
    QDomElement asGML2( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const override;
    QDomElement asGML3( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const override;
    QString asJSON( int precision = 17 ) const override;

    //! Adds a geometry and takes ownership. Returns true in case of success
    virtual bool addGeometry( QgsAbstractGeometry* g ) override;

    /** Returns a copy of the multi curve, where each component curve has had its line direction reversed.
     * @note added in QGIS 2.14
     */
    QgsMultiCurve* reversed() const;

    virtual QgsAbstractGeometry* boundary() const override;

};

#endif // QGSMULTICURVEV2_H
