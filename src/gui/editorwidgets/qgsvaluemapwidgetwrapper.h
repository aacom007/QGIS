/***************************************************************************
    qgsvaluemapwidgetwrapper.h
     --------------------------------------
    Date                 : 5.1.2014
    Copyright            : (C) 2014 Matthias Kuhn
    Email                : matthias at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVALUEMAPWIDGETWRAPPER_H
#define QGSVALUEMAPWIDGETWRAPPER_H

#include "qgseditorwidgetwrapper.h"

#include <QComboBox>
#include "qgis_gui.h"


/** \ingroup gui
 * Wraps a value map widget.
 *
 * Options:
 * <ul>
 * <li><b>[Key]</b> <i>Value</i></li>
 * </ul>
 *
 * Any option will be treated as entry in the value map.
 * \note not available in Python bindings
 */

class GUI_EXPORT QgsValueMapWidgetWrapper : public QgsEditorWidgetWrapper
{
    Q_OBJECT
  public:
    explicit QgsValueMapWidgetWrapper( QgsVectorLayer* vl, int fieldIdx, QWidget* editor = nullptr, QWidget* parent = nullptr );

    // QgsEditorWidgetWrapper interface
  public:
    QVariant value() const override;
    void showIndeterminateState() override;

  protected:
    QWidget* createWidget( QWidget* parent ) override;
    void initWidget( QWidget* editor ) override;
    bool valid() const override;

  public slots:
    void setValue( const QVariant& value ) override;

  private:
    QComboBox* mComboBox = nullptr;
};

#endif // QGSVALUEMAPWIDGETWRAPPER_H
