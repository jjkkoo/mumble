/* Copyright (C) 2005-2009, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QtXml>

#include "Usage.h"
#include "Version.h"
#include "Global.h"

Usage::Usage(QObject *parent) : QObject(parent) {
	// Wait 10 minutes (so we know they're actually using this), then...
	QTimer::singleShot(60 * 10 * 1000, this, SLOT(registerUsage()));
}

void Usage::registerUsage() {
	if (! g.s.bUsage || g.s.bFirstTime)
		return;

	QDomDocument doc;
	QDomElement root=doc.createElement(QLatin1String("usage"));
	doc.appendChild(root);

	QDomElement tag;
	QDomText t;

	QString machash;

	foreach(const QNetworkInterface &qni, QNetworkInterface::allInterfaces()) {
		if (! qni.isValid())
			continue;
		if (! (qni.flags() & (QNetworkInterface::IsUp | QNetworkInterface::IsRunning)))
			continue;
		if (qni.hardwareAddress().isEmpty())
			continue;

		bool found = false;
		foreach(const QNetworkAddressEntry &qnae, qni.addressEntries()) {
			const QHostAddress &qha = qnae.ip();
			if (qha.protocol() ==  QAbstractSocket::IPv4Protocol) {
				found = true;
			}
		}
		if (found) {
			machash = QString::fromAscii(QCryptographicHash::hash(qni.hardwareAddress().toAscii(), QCryptographicHash::Sha1).toHex());
		}
	}

	tag=doc.createElement(QLatin1String("machash"));
	root.appendChild(tag);
	t=doc.createTextNode(machash);
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("version"));
	root.appendChild(tag);
	t=doc.createTextNode(QLatin1String(MUMTEXT(MUMBLE_VERSION_STRING)));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("release"));
	root.appendChild(tag);
	t=doc.createTextNode(QLatin1String(MUMBLE_RELEASE));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("os"));
	root.appendChild(tag);
#if defined(Q_WS_WIN)
	t=doc.createTextNode(QLatin1String("Win"));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("osver"));
	root.appendChild(tag);
	t=doc.createTextNode(QString::number(QSysInfo::WindowsVersion, 16));
	tag.appendChild(t);
#elif defined(Q_WS_MAC)
	t=doc.createTextNode(QLatin1String("OSX"));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("osver"));
	root.appendChild(tag);
	t=doc.createTextNode(QString::number(QSysInfo::MacintoshVersion, 16));
	tag.appendChild(t);
#else
	t=doc.createTextNode(QLatin1String("X11"));
	tag.appendChild(t);
#endif

	bool bIs64;
#if defined(Q_WS_WIN)
	BOOL bIsWow64 = FALSE;
	IsWow64Process(GetCurrentProcess(), &bIsWow64);
	bIs64 = bIsWow64;
#else
	bIs64 = (QSysInfo::WordSize == 64);
#endif

	tag=doc.createElement(QLatin1String("is64bit"));
	root.appendChild(tag);
	t=doc.createTextNode(QString::number(bIs64 ? 1 : 0));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("qt"));
	root.appendChild(tag);
	t=doc.createTextNode(QLatin1String(qVersion()));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("in"));
	root.appendChild(tag);
	t=doc.createTextNode(g.s.qsAudioInput);
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("out"));
	root.appendChild(tag);
	t=doc.createTextNode(g.s.qsAudioOutput);
	tag.appendChild(t);

	qhHttp.setHost(QLatin1String("mumble.hive.no"), 80);

	QHttpRequestHeader h(QLatin1String("POST"), QLatin1String("/usage.cgi"));
	h.setValue(QLatin1String("Host"), QLatin1String("mumble.hive.no"));
	h.setContentType(QLatin1String("text/xml"));
	qhHttp.request(h, doc.toString().toUtf8());
	qhHttp.close();
}
