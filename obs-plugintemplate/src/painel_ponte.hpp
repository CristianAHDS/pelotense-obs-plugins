#pragma once

#include <QDockWidget>
#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>

class PainelPonte : public QObject {
	QPointer<QDockWidget> dock_;
	std::function<void(const QString &)> onTitulo_;

public:
	explicit PainelPonte(QObject *pai = nullptr) : QObject(pai) {}

	void definirDock(QDockWidget *dock) { dock_ = dock; }
	void definirOnTitulo(std::function<void(const QString &)> cb) { onTitulo_ = cb; }

	void aplicarTitulo(const QString &titulo)
	{
		if (!dock_)
			return;
		QString limpo = titulo.trimmed();
		if (limpo.isEmpty() || limpo.startsWith(QLatin1String("http")))
			return;
		QString completo = limpo;
		dock_->setWindowTitle(completo);
		if (onTitulo_)
			onTitulo_(completo);
	}
};
