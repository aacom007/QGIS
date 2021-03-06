/***************************************************************************
                         qgssinglebandgrayrenderer.h
                         ---------------------------
    begin                : December 2011
    copyright            : (C) 2011 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSINGLEBANDGRAYRENDERER_H
#define QGSSINGLEBANDGRAYRENDERER_H

#include "qgis_core.h"
#include "qgsrasterrenderer.h"

class QgsContrastEnhancement;
class QDomElement;

/** \ingroup core
  * Raster renderer pipe for single band gray.
  */
class CORE_EXPORT QgsSingleBandGrayRenderer: public QgsRasterRenderer
{
  public:
    enum Gradient
    {
      BlackToWhite,
      WhiteToBlack
    };

    QgsSingleBandGrayRenderer( QgsRasterInterface* input, int grayBand );
    ~QgsSingleBandGrayRenderer();

    //! QgsSingleBandGrayRenderer cannot be copied. Use clone() instead.
    QgsSingleBandGrayRenderer( const QgsSingleBandGrayRenderer& ) = delete;
    //! QgsSingleBandGrayRenderer cannot be copied. Use clone() instead.
    const QgsSingleBandGrayRenderer& operator=( const QgsSingleBandGrayRenderer& ) = delete;

    QgsSingleBandGrayRenderer * clone() const override;

    static QgsRasterRenderer* create( const QDomElement& elem, QgsRasterInterface* input );

    QgsRasterBlock *block( int bandNo, QgsRectangle  const & extent, int width, int height, QgsRasterBlockFeedback* feedback = nullptr ) override;

    int grayBand() const { return mGrayBand; }
    void setGrayBand( int band ) { mGrayBand = band; }
    const QgsContrastEnhancement* contrastEnhancement() const { return mContrastEnhancement; }
    //! Takes ownership
    void setContrastEnhancement( QgsContrastEnhancement* ce );

    void setGradient( Gradient theGradient ) { mGradient = theGradient; }
    Gradient gradient() const { return mGradient; }

    void writeXml( QDomDocument& doc, QDomElement& parentElem ) const override;

    void legendSymbologyItems( QList< QPair< QString, QColor > >& symbolItems ) const override;

    QList<int> usesBands() const override;

  private:
    int mGrayBand;
    Gradient mGradient;
    QgsContrastEnhancement* mContrastEnhancement = nullptr;

};

#endif // QGSSINGLEBANDGRAYRENDERER_H
