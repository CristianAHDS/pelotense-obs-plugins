#include "pelotense-assets.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <util/platform.h>

#include <QDockWidget>
#include <QFont>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QPainter>
#include <QPen>
#include <QPointer>
#include <QPixmap>
#include <QPolygon>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QSize>
#include <QColor>
#include <QEasingCurve>
#include <QMessageBox>
#include <obs-data.h>
#include <obs-source.h>

#include "browser-panel.hpp"
#include "painel_ponte.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define DOCK_ID_ASSETS "pelotense_assets_browser_dock"
#define DOCK_TITULO_ASSETS "Pelotense Assets"
#define URL_PADRAO_ASSETS "http://192.168.8.231:5174/testes"
#define URL_SUB_DE_PADRAO "https://pelotense-assets.netlify.app"
#define URL_SUB_PARA_PADRAO "http://192.168.8.231:5174"
#define ARQUIVO_SUB_URLS "substituir-urls.txt"
#define MAX_FONTES_COLETA 1024
#define ACENTO_COR "#6366f1"
#define MAX_TENTATIVAS 120

static QCef *cef_panel = nullptr;

struct Painel {
	QPointer<QDockWidget> dock;
	QPointer<QCefWidget> navegador;
	QPointer<PainelPonte> ponte;
	char url[2048];
	bool esteve_oculto = false;
};

static Painel painel_assets;



static void ler_urls(void)
{
	snprintf(painel_assets.url, sizeof(painel_assets.url), "%s", URL_PADRAO_ASSETS);

	char *caminho = obs_module_config_path("url.txt");
	if (caminho) {
		FILE *arquivo = fopen(caminho, "r");
		if (arquivo) {
			char linha[2048];
			if (fgets(linha, sizeof(linha), arquivo)) {
				size_t tamanho = strlen(linha);
				while (tamanho &&
				       (linha[tamanho - 1] == '\n' || linha[tamanho - 1] == '\r'))
					linha[--tamanho] = 0;
				if (linha[0])
					snprintf(painel_assets.url, sizeof(painel_assets.url), "%s",
						 linha);
			}
			fclose(arquivo);
		}
		bfree(caminho);
	}

	char *env = getenv("PELOTESE_ASSETS_URL");
	if (env && *env)
		snprintf(painel_assets.url, sizeof(painel_assets.url), "%s", env);
}

	struct FonteColeta {
		obs_source_t *fontes[MAX_FONTES_COLETA];
		int total;
	};

	static bool coletar_fontes_proc(void *param, obs_source_t *source)
	{
		FonteColeta *c = (FonteColeta *)param;
		if (!source || c->total >= MAX_FONTES_COLETA)
			return true;
		const char *id = obs_source_get_id(source);
		if (!id || strcmp(id, "browser_source") != 0)
			return true;
		c->fontes[c->total++] = source;
		return true;
	}

	static void ler_substituir_urls(char *de, size_t de_tam, char *para, size_t para_tam)
	{
		snprintf(de, de_tam, "%s", URL_SUB_DE_PADRAO);
		snprintf(para, para_tam, "%s", URL_SUB_PARA_PADRAO);

		char *caminho = obs_module_config_path(ARQUIVO_SUB_URLS);
		if (caminho) {
			FILE *arquivo = fopen(caminho, "r");
			if (arquivo) {
				char linha[2048];
				int lida = 0;
				while (fgets(linha, sizeof(linha), arquivo) && lida < 2) {
					size_t tamanho = strlen(linha);
					while (tamanho &&
					       (linha[tamanho - 1] == '\n' || linha[tamanho - 1] == '\r'))
						linha[--tamanho] = 0;
					char *igual = strchr(linha, '=');
					if (igual) {
						char *valor = igual + 1;
						while (*valor == ' ')
							valor++;
						if (lida == 0)
							snprintf(de, de_tam, "%s", valor);
						else if (lida == 1)
							snprintf(para, para_tam, "%s", valor);
						lida++;
					} else if (tamanho > 0) {
						if (lida == 0)
							snprintf(de, de_tam, "%s", linha);
						else if (lida == 1)
							snprintf(para, para_tam, "%s", linha);
						lida++;
					}
				}
				fclose(arquivo);
			}
			bfree(caminho);
		}
	}

	static int substituir_urls_todas_fontes(void)
	{
		char de[2048], para[2048];
		ler_substituir_urls(de, sizeof(de), para, sizeof(para));

		FonteColeta coleta = {};
		coleta.total = 0;
		obs_enum_sources(coletar_fontes_proc, &coleta);

		int alteradas = 0;
		size_t de_len = strlen(de);
		for (int i = 0; i < coleta.total; i++) {
			obs_source_t *src = coleta.fontes[i];
			obs_data_t *settings = obs_source_get_settings(src);
			if (!settings) {
				obs_source_release(src);
				continue;
			}
			const char *url = obs_data_get_string(settings, "url");
			if (url && *url && strncmp(url, de, de_len) == 0) {
				char nova[4096];
				snprintf(nova, sizeof(nova), "%s%s", para, url + de_len);
				obs_data_set_string(settings, "url", nova);
				obs_source_update(src, settings);
				alteradas++;
			}
			obs_data_release(settings);
			obs_source_release(src);
		}

		blog(LOG_INFO, "[PelotenseAssets] %d de %d fonte(s) de navegador atualizada(s) (%s -> %s).",
		     alteradas, coleta.total, de, para);
		return alteradas;
	}

static void conectar_titulo(Painel *p)
{
	if (!p->ponte || !p->navegador)
		return;
	p->ponte->definirOnTitulo([p](const QString &titulo) {
		if (p->dock)
			p->dock->setWindowTitle(titulo);
	});
	QObject::connect(p->navegador.data(), SIGNAL(titleChanged(const QString &)),
			 p->ponte.data(), SLOT(aplicarTitulo(const QString &)));
}

static void recriar_widget(Painel *p)
{
	QDockWidget *dock = p->dock.data();
	if (!dock)
		return;

	QCefWidget *novo = cef_panel->create_widget(dock, p->url);
	if (novo) {
		dock->setWidget(novo);
		p->navegador = novo;
		conectar_titulo(p);
	}
}

static void centralizar_painel(Painel *p)
{
	QDockWidget *dock = p->dock.data();
	if (!dock)
		return;

	QMainWindow *janela = (QMainWindow *)obs_frontend_get_main_window();
	QScreen *tela = janela ? QGuiApplication::screenAt(janela->geometry().center()) : nullptr;
	if (tela) {
		QRect area = tela->availableGeometry();
		QSize tamanho = QSize((int)(area.width() * 0.9f), (int)(area.height() * 0.9f));
		dock->resize(tamanho);
		QRect geo(area.center().x() - tamanho.width() / 2,
			  area.center().y() - tamanho.height() / 2, tamanho.width(),
			  tamanho.height());
		geo = geo.intersected(area);
		dock->setGeometry(geo);
	}
}

static QColor misturar(const QColor &a, const QColor &b, qreal t)
{
	return QColor(qRound(a.red() + (b.red() - a.red()) * t),
		      qRound(a.green() + (b.green() - a.green()) * t),
		      qRound(a.blue() + (b.blue() - a.blue()) * t));
}

static QString qss_botao(const QColor &cor, const QColor &checked)
{
	return QString("QPushButton{background:%1;color:#0a0a0f;border:none;"
		       "border-radius:3px;padding:6px;}"
		       "QPushButton:hover{background:%2;}"
		       "QPushButton:pressed{background:%3;}"
		       "QPushButton:checked{background:%4;}"
		       "QPushButton:checked:hover{background:%5;}")
		.arg(cor.name())
		.arg(misturar(cor, QColor("#ffffff"), 0.14).name())
		.arg(misturar(cor, QColor("#000000"), 0.16).name())
		.arg(checked.name())
		.arg(misturar(checked, QColor("#ffffff"), 0.18).name());
}

static QIcon icone_assets(void)
{
	QPixmap pm(16, 16);
	pm.fill(Qt::transparent);

	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(QPen(QColor("#151533"), 1.4));
	p.setBrush(QColor(ACENTO_COR));
	p.drawRoundedRect(QRectF(2.0, 2.0, 12.0, 12.0), 2.5, 2.5);

	p.setBrush(QColor("#0a0a1a"));
	p.drawRect(QRectF(6.0, 6.0, 4.0, 4.0));
	p.end();

	return QIcon(pm);
}

static void estilizar_pulso_botao(QPushButton *botao, qreal t)
{
	QColor alvo = misturar(QColor(ACENTO_COR), QColor("#c7c9ff"), t);
	botao->setStyleSheet(qss_botao(QColor(ACENTO_COR), alvo));
}

static void pulso_parar(QPushButton *botao)
{
	QVariantAnimation *anim = botao->findChild<QVariantAnimation *>();
	if (anim) {
		anim->stop();
		delete anim;
	}
	estilizar_pulso_botao(botao, 0.0);
}

static void pulso_iniciar(QPushButton *botao)
{
	if (botao->findChild<QVariantAnimation *>())
		return;

	QVariantAnimation *anim = new QVariantAnimation(botao);
	anim->setDuration(1000);
	anim->setStartValue(0.0);
	anim->setEndValue(1.0);
	anim->setKeyValueAt(0.0, 0.0);
	anim->setKeyValueAt(0.5, 1.0);
	anim->setKeyValueAt(1.0, 0.0);
	anim->setLoopCount(-1);
	anim->setEasingCurve(QEasingCurve::InOutSine);
	QObject::connect(anim, &QVariantAnimation::valueChanged, botao, [botao](const QVariant &v) {
		estilizar_pulso_botao(botao, v.toReal());
	});
	anim->start();
}

static void estilizar_botao(QPushButton *botao)
{
	QFont fonte("Inter", 10);
	fonte.setBold(true);
	botao->setFont(fonte);

	botao->setIcon(icone_assets());
	botao->setIconSize(QSize(14, 14));
	botao->setStyleSheet(qss_botao(QColor(ACENTO_COR), QColor("#4f52d1")));
}

static void configurar_painel(Painel *p, const char *id, const char *titulo, QCef *cef)
{
	QWidget *pai = (QWidget *)obs_frontend_get_main_window();
	p->navegador = cef->create_widget(pai, p->url);
	if (!p->navegador) {
		blog(LOG_WARNING, "[%s] Falha ao criar widget para %s.", "PelotenseAssets", titulo);
		return;
	}

	if (!obs_frontend_add_dock_by_id(id, titulo, p->navegador.data())) {
		delete p->navegador.data();
		p->navegador = nullptr;
		return;
	}

	QMainWindow *janela = (QMainWindow *)pai;
	p->dock = janela ? janela->findChild<QDockWidget *>(id) : nullptr;
	QDockWidget *dock = p->dock.data();
		if (dock) {
		dock->setMinimumSize(400, 300);
		dock->setFloating(true);
		QScreen *tela_cfg = janela ? QGuiApplication::screenAt(janela->geometry().center()) : nullptr;
		if (tela_cfg) {
			QRect area_cfg = tela_cfg->availableGeometry();
			dock->resize((int)(area_cfg.width() * 0.9f), (int)(area_cfg.height() * 0.9f));
		} else {
			dock->resize((int)(janela->width() * 0.9f), (int)(janela->height() * 0.9f));
		}

		p->ponte = new PainelPonte(dock);
		p->ponte->definirDock(dock);
		conectar_titulo(p);

		centralizar_painel(p);

		QObject::connect(dock, &QDockWidget::visibilityChanged, dock, [p](bool visivel) {
			if (visivel) {
				if (p->esteve_oculto)
					recriar_widget(p);
			} else {
				p->esteve_oculto = true;
			}
		});
	}

	blog(LOG_INFO, "[PelotenseAssets] Painel criado: %s (%s)", "PelotenseAssets", titulo, p->url);
}

static void adicionar_botao_controles(void)
{
	if (!painel_assets.dock)
		return;

	QMainWindow *janela = (QMainWindow *)obs_frontend_get_main_window();
	if (!janela)
		return;

	QDockWidget *controles = janela->findChild<QDockWidget *>("controlsDock");
	if (!controles)
		return;

	QWidget *frame = controles->findChild<QWidget *>("controlsFrame");
	if (!frame)
		return;

	QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(frame->layout());
	if (!layout)
		return;

	QPushButton *botao = new QPushButton("Pelotense Assets");
	botao->setCheckable(true);
	botao->setAutoDefault(false);
	botao->setChecked(painel_assets.dock->isVisible());
	botao->setToolTip("Abrir/fechar painel Assets (https://www.google.com)");
	estilizar_botao(botao);

	QObject::connect(botao, &QPushButton::toggled, frame, [](bool checado) {
		painel_assets.dock->setVisible(checado);
	});
	QObject::connect(botao, &QPushButton::toggled, frame, [botao](bool checado) {
		if (checado)
			pulso_iniciar(botao);
		else
			pulso_parar(botao);
	});
	QObject::connect(painel_assets.dock.data(), &QDockWidget::visibilityChanged, frame,
			 [botao](bool visivel) { botao->setChecked(visivel); });

	QWidget *bloco = new QWidget(frame);
	QVBoxLayout *coluna = new QVBoxLayout(bloco);
	coluna->setSpacing(4);
	coluna->setContentsMargins(0, 0, 0, 0);

	QLabel *rotulo = new QLabel(bloco);
	rotulo->setTextFormat(Qt::RichText);
	rotulo->setText(QString("<span style='color:%1;font-weight:bold;'>PELOTENSE</span> ")
			     .arg(ACENTO_COR)
			     .append("<span style='color:#dcdcdc;font-weight:bold;'>ASSETS</span>"));
	rotulo->setStyleSheet(QString("border-left:3px solid %1;padding-left:6px;"
				      "background:transparent;")
				      .arg(ACENTO_COR));
	coluna->addWidget(rotulo);
	coluna->addWidget(botao);

	QPushButton *botao_urls = new QPushButton("Atualizar URLs das Fontes");
	botao_urls->setAutoDefault(false);
	estilizar_botao(botao_urls);
	QObject::connect(botao_urls, &QPushButton::clicked, [botao_urls]() {
		int alteradas = substituir_urls_todas_fontes();
		QMainWindow *janela = (QMainWindow *)obs_frontend_get_main_window();
		QString msg;
		if (alteradas == 1)
			msg = "1 fonte de navegador atualizada.";
		else
			msg = QString("%1 fontes de navegador atualizadas.").arg(alteradas);
		QMessageBox::information(janela, "Pelotense Assets - URLs", msg);
		botao_urls->setText(msg);
	});
	coluna->addWidget(botao_urls);

	layout->addWidget(bloco);
}

static void criar_paineis(int tentativa)
{
	if (!cef_panel)
		cef_panel = obs_browser_init_panel();

	if (!cef_panel) {
		blog(LOG_WARNING, "[PelotenseAssets] obs-browser nao encontrado.");
		return;
	}

	if (!cef_panel->initialized()) {
		cef_panel->init_browser();

		if (tentativa >= MAX_TENTATIVAS) {
			blog(LOG_WARNING, "[PelotenseAssets] CEF nao inicializou a tempo.");
			return;
		}
		QTimer::singleShot(500, [tentativa]() { criar_paineis(tentativa + 1); });
		return;
	}

	configurar_painel(&painel_assets, DOCK_ID_ASSETS, DOCK_TITULO_ASSETS, cef_panel);

	adicionar_botao_controles();
}

static void ao_evento(enum obs_frontend_event evento, void *)
{
	if (evento == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		ler_urls();
		criar_paineis(0);
	}
}

extern "C" void pelotense_assets_init(void)
{
	obs_frontend_add_event_callback(ao_evento, nullptr);
}

extern "C" void pelotense_assets_deinit(void)
{
	obs_frontend_remove_event_callback(ao_evento, nullptr);
	obs_frontend_remove_dock(DOCK_ID_ASSETS);
}
