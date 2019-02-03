/*
    ClipGrab³
    Copyright (C) Philipp Schmieder
    http://clipgrab.de
    feedback [at] clipgrab [dot] de

    This file is part of ClipGrab.
    ClipGrab is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ClipGrab is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ClipGrab.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "video_dailymotion.h"

video_dailymotion::video_dailymotion()
{
    this->_name = "Dailymotion";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("http[s]?://\\w*\\.dailymotion\\.com/video/([^?/]+)", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://dai\\.ly/([^?/]+)", Qt::CaseInsensitive);
}

video* video_dailymotion::createNewInstance()
{
    return new video_dailymotion();
}

bool video_dailymotion::setUrl(QString url)
{
    _originalUrl = url;

    for (int i = 0; i < this->_urlRegExp.length(); i++)
    {
        if (this->_urlRegExp.at(i).indexIn((url)) == -1)
        {
            continue;
        }

        QString videoId = this->_urlRegExp.at(i).cap(1);
        this->_url = QUrl("https://www.dailymotion.com/player/metadata/video/" + videoId);

        QNetworkCookieJar* cookieJar = new QNetworkCookieJar;
        QList<QNetworkCookie> cookieList;
        cookieList << QNetworkCookie("ff", "off");
        cookieJar->setCookiesFromUrl(cookieList, _url);
        this->handler->networkAccessManager->setCookieJar(cookieJar);

        return true;
    }
    return false;
}

void video_dailymotion::parseVideo(QString json)
{
    _title = this->getProperty(json, "title");

    QList<dailymotion_quality> qualities;
    qualities << dailymotion_quality("1080", tr("HD (1080p)"));
    qualities << dailymotion_quality("720", tr("HD (720p)"));
    qualities << dailymotion_quality("480", tr("480p"));
    qualities << dailymotion_quality("380", tr("380p"));
    qualities << dailymotion_quality("240", tr("240p"));

    QListIterator<dailymotion_quality> i(qualities);
    while (i.hasNext())
    {
        dailymotion_quality quality = i.next();
        QString url = getQualityUrl(json, quality.key);
        if (!url.isEmpty())
        {
            videoQuality newQuality;
            newQuality.quality = quality.name;
            newQuality.videoUrl = url;
            newQuality.containerName = url.contains(".mp4") ? ".mp4" : ".flv";
            newQuality.resolution = quality.key.toInt();
            _supportedQualities.append(newQuality);
        }
    }

    if (_supportedQualities.isEmpty() | _title.isEmpty())
    {
        emit error("Could not retrieve video title.", this);

    }
    emit analysingFinished();
 }

QString video_dailymotion::getProperty(QString json, QString property) {
    QWebView* view = new QWebView();
    QString script;

    script.append( "function getProperty(property) {\n");
    script.append( "  var json = " + json + ";\n");
    script.append( "  return json[property];\n");
    script.append( "}\n");
    view->setHtml("<script>" + script + "</script>");
    QString result = view->page()->mainFrame()->evaluateJavaScript("getProperty(\"" + property +"\")").toString();

    view->deleteLater();

    return result;
}

QString video_dailymotion::getQualityUrl(QString json, QString quality)
{
    QWebView* view = new QWebView();
    QString script;
    script.append( "function getUrl() {\n");
    script.append( "  var json = " + json + ";\n");
    script.append( "  var sources = json.qualities['" + quality + "'];\n");
    script.append( "  for (var i in sources) {\n");
    script.append( "    if (sources[i].type == 'video\\/mp4') {\n");
    script.append( "        return sources[i].url\n");
    script.append( "    }\n");
    script.append( "  }\n");
    script.append( "}\n");
    view->setHtml("<script>" + script + "</script>");
    QString result = view->page()->mainFrame()->evaluateJavaScript("getUrl()").toString();

    view->deleteLater();
    return result;
}
