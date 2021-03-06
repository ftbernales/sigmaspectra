////////////////////////////////////////////////////////////////////////////////////
// This file is part of SigmaSpectra.
//
// SigmaSpectra is free software: you can redistribute it and/or modify it under
// the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// SigmaSpectra is distributed in the hope that it will be useful, but WITHOUT
// ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A
// PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// SigmaSpectra.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2008-2017 Albert Kottke
////////////////////////////////////////////////////////////////////////////////////

#include "MyTableView.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDomDocument>
#include <QMimeData>

MyTableView::MyTableView(QWidget *parent)
        : QTableView(parent) {
    // Create the context menu
    m_contextMenu = new QMenu;
    m_contextMenu->addAction(QIcon(":/images/edit-copy.svg"), tr("Copy"), this,
                             SLOT(copy()), QKeySequence(tr("Ctrl+c")));
    m_contextMenu->addAction(QIcon(":/images/edit-paste.svg"), tr("Paste"), this,
                             SLOT(paste()), QKeySequence(tr("Ctrl+v")));
}

void MyTableView::copy() {
    QString data;

    // Determine the currently selected rows
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();

    // Sort the indexes
    std::sort(selectedIndexes.begin(), selectedIndexes.end());

    for (int i = 0; i < selectedIndexes.size(); ++i) {
        // Add the data in the cell
        data += model()->data(selectedIndexes.at(i)).toString();

        if (i + 1 < selectedIndexes.size()) {
            if (selectedIndexes.at(i).row() != selectedIndexes.at(i + 1).row()) {
                // Add a line break when the row changes
                data += "\n";
            } else {
                // Add a tab to separate values on the same row
                data += "\t";
            }
        }
    }

    // Set the text of the clipboard
    QApplication::clipboard()->setText(data);
}

void MyTableView::paste() {
    bool ok;
    QList<QList<double>> data;

    bool hasHtml = QApplication::clipboard()->mimeData()->hasHtml();
    bool htmlValid = true;

    if (hasHtml) {
        QDomDocument doc;
        if (doc.setContent(QApplication::clipboard()->mimeData()->html())) {

            // All rows of the table
            QDomNodeList nodeList = doc.elementsByTagName("tr");

            if (nodeList.isEmpty()) {
                return;
            }

            for (int i = 0; i < nodeList.size(); ++i) {
                data << QList<double>();

                // All columns of the table
                QDomElement e = nodeList.at(i).firstChildElement("td");

                // Grab the data for each of the columns
                while (e.isNull() == false) {
                    data.last() << e.text().toDouble(&ok);
                    if (ok == false) {
                        qWarning("Error converting \"%s\" to a number", qPrintable(e.text()));
                        return;
                    }
                    e = e.nextSiblingElement();
                }
            }
        } else {
            htmlValid = false;
        }
    }

    if (hasHtml == false || htmlValid == false) {
        // Grab the text from the clipboard and split it into lines
        QStringList rows = QApplication::clipboard()->text().split(
                QRegExp("\\n"), QString::SkipEmptyParts);

        // Return if the row list is empty
        if (rows.size() == 0)
            return;
        for (const QString &row : rows) {
            data << QList<double>();
            // Split the row text into cells based on tabs
            QStringList cells = row.split(QRegExp("\\s"), QString::SkipEmptyParts);
            for (QString &cell : cells) {
                cell = cell.trimmed();
                data.last() << cell.toDouble(&ok);
                if (ok == false) {
                    qWarning("Error converting \"%s\" to a number", qPrintable(cell));
                    return;
                }
            }
        }
    }

    // Get the currently selected items
    QModelIndex initial = currentIndex();

    // If the number of rows is greater than the number of rows beyond the
    // current index and the required number of rows.
    if (initial.row() < 0) {
        // No row selected
        model()->insertRows(model()->rowCount(), data.size() - model()->rowCount());
        initial = model()->index(0, 0);
    } else if (model()->rowCount() < initial.row() + data.size()) {
        model()->insertRows(model()->rowCount(),
                            initial.row() + data.size() - model()->rowCount());
    }

    // Stop the model from updating the view
    model()->blockSignals(true);
    // Set the data in the rows
    for (int i = 0; i < data.size(); ++i) {
        for (int j = 0; j < data.at(i).size(); ++j) {
            // Skip if the string is empty
            model()->setData(initial.sibling(initial.row() + i, initial.column() + j),
                             data.at(i).at(j), Qt::EditRole);
        }
    }
    // Re-enable the model to send signals
    model()->blockSignals(false);

    // Signal that the data has changed
    dataChanged(
            initial.sibling(initial.row(), initial.column()),
            initial.sibling(initial.row() + data.size(), model()->columnCount()));

    resizeColumnsToContents();
    resizeRowsToContents();

    emit dataPasted();
}

void MyTableView::contextMenuEvent(QContextMenuEvent *event) {
    m_contextMenu->popup(event->globalPos());
}
