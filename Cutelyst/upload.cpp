/*
 * Copyright (C) 2014 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "upload_p.h"
#include "common.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QStringBuilder>

using namespace Cutelyst;

QString Upload::filename() const
{
    Q_D(const Upload);
    return d->filename;
}

QByteArray Upload::contentType() const
{
    Q_D(const Upload);
    return d->headers.value("Content-Type");
}

bool Upload::save(const QString &newName)
{
    Q_D(Upload);
    bool error = false;
    QString fileTemplate = QLatin1String("%1/qt_temp.XXXXXX");
#ifdef QT_NO_TEMPORARYFILE
    QFile out(fileTemplate.arg(QFileInfo(newName).path()));
    if (!out.open(QIODevice::ReadWrite)) {
        error = true;
    }
#else
    QTemporaryFile out(fileTemplate.arg(QFileInfo(newName).path()));
    if (!out.open()) {
        out.setFileTemplate(fileTemplate.arg(QDir::tempPath()));
        if (!out.open()) {
            error = true;
        }
    }
#endif

    if (error) {
        out.close();
        setErrorString(QLatin1String("Failed to open file for saving: ") % out.errorString());
        qCWarning(CUTELYST_UPLOAD) << errorString();
    } else {
        qint64 posOrig = d->pos;
        seek(0);

        char block[4096];
        qint64 totalRead = 0;
        while (!atEnd()) {
            qint64 in = read(block, sizeof(block));
            if (in <= 0)
                break;
            totalRead += in;
            if (in != out.write(block, in)) {
                setErrorString(QLatin1String("Failure to write block"));
                qCWarning(CUTELYST_UPLOAD) << errorString();
                error = true;
                break;
            }
        }

        if (error) {
            out.remove();
        }

        if (!error && !out.rename(newName)) {
            error = true;
            setErrorString(QString("Cannot create %1 for output").arg(newName));
            qCWarning(CUTELYST_UPLOAD) << errorString();
        }
#ifdef QT_NO_TEMPORARYFILE
        if (error) {
            out.remove();
        }
#else
        if (!error) {
            out.setAutoRemove(false);
        }
#endif
        seek(posOrig);
    }

    return !error;
}

qint64 Upload::pos() const
{
    Q_D(const Upload);
    return d->pos;
}

qint64 Upload::size() const
{
    Q_D(const Upload);
    return d->endOffset - d->startOffset;
}

bool Upload::seek(qint64 pos)
{
    Q_D(Upload);
    if (pos <= size()) {
        d->pos = pos;
        return true;
    }
    return false;
}

Upload::Upload(UploadPrivate *prv) :
    d_ptr(prv)
{
    open(prv->device->openMode());
}

qint64 Upload::readData(char *data, qint64 maxlen)
{
    Q_D(Upload);
    qint64 posOrig = d->device->pos();
    qint64 remains = size() - d->pos;
    if (maxlen < remains) {
        remains = maxlen;
    }

    d->device->seek(d->startOffset + d->pos);
    qint64 len = d->device->read(data, remains);
    d->device->seek(posOrig);
    d->pos += len;
    return len;
}

qint64 Upload::readLineData(char *data, qint64 maxlen)
{
    Q_D(Upload);
    qint64 posOrig = d->device->pos();
    qint64 remains = size() - d->pos;
    if (maxlen < remains) {
        remains = maxlen;
    }

    d->device->seek(d->startOffset + d->pos);
    qint64 len = d->device->readLine(data, remains);
    d->device->seek(posOrig);
    d->pos += len;
    return len;
}

qint64 Upload::writeData(const char *data, qint64 maxSize)
{
    return -1;
}


UploadPrivate::UploadPrivate(QIODevice *dev) :
    device(dev)
{

}
