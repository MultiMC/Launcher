#include "AnalyticsWizardPage.h"
#include <Application.h>

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QCheckBox>

#include <ganalytics.h>
#include <BuildConfig.h>

AnalyticsWizardPage::AnalyticsWizardPage(QWidget *parent)
    : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("analyticsPage"));
    verticalLayout_3 = new QVBoxLayout(this);
    verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
    textBrowser = new QTextBrowser(this);
    textBrowser->setObjectName(QStringLiteral("textBrowser"));
    textBrowser->setAcceptRichText(false);
    textBrowser->setOpenExternalLinks(true);
    verticalLayout_3->addWidget(textBrowser);

    checkBox = new QCheckBox(this);
    checkBox->setObjectName(QStringLiteral("checkBox"));
    checkBox->setChecked(true);
    verticalLayout_3->addWidget(checkBox);
    retranslate();
}

AnalyticsWizardPage::~AnalyticsWizardPage()
{
}

bool AnalyticsWizardPage::validatePage()
{
    auto settings = APPLICATION->settings();
    auto analytics = APPLICATION->analytics();
    auto status = checkBox->isChecked();
    settings->set("AnalyticsSeen", analytics->version());
    settings->set("Analytics", status);
    return true;
}

void AnalyticsWizardPage::retranslate()
{
    setTitle(tr("Analytics"));
    setSubTitle(tr("We track some anonymous statistics about users."));
    textBrowser->setHtml(tr(
        "<html><body>"
        "<p>The launcher sends anonymous usage statistics on every start of the application. This helps us decide what platforms and issues to focus on.</p>"
        "<p>The data is processed by Google Analytics, see their <a href=\"https://support.google.com/analytics/answer/6004245?hl=en\">article on the "
        "matter</a>.</p>"
        "<p>The following data is collected:</p>"
        "<ul><li>A random unique ID of the installation.<br />It is stored in the application settings file.</li>"
        "<li>Anonymized (partial) IP address.</li>"
        "<li>Launcher version.</li>"
        "<li>Operating system name, version and architecture.</li>"
        "<li>CPU architecture (kernel architecture on linux).</li>"
        "<li>Size of system memory.</li>"
        "<li>Java version, architecture and memory settings.</li></ul>"
        "<p>If we change the tracked information, you will see this page again.</p></body></html>"));
    checkBox->setText(tr("Enable Analytics"));
}
