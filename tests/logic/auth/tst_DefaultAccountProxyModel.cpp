#include "ModelTester.h"

#include "logic/auth/DefaultAccountProxyModel.h"
#include "logic/auth/AccountModel.h"
#include "logic/auth/BaseAccountType.h"
#include "logic/minecraft/auth/MojangAccount.h"

class DefaultAccountProxyModelTest : public ModelTester
{
	Q_OBJECT
public:
	std::shared_ptr<QAbstractItemModel> createModel() const override
	{
		auto m = std::make_shared<DefaultAccountProxyModel>(nullptr);
		m->setSourceModel(new AccountModel);
		return m;
	}
	void populate(std::shared_ptr<QAbstractItemModel> model) const override
	{
		auto m = dynamic_cast<AccountModel *>(std::dynamic_pointer_cast<DefaultAccountProxyModel>(model)->sourceModel());
		m->registerAccount(m->createAccount<MojangAccount>());
	}
};

QTEST_GUILESS_MAIN(DefaultAccountProxyModelTest)

#include "tst_DefaultAccountProxyModel.moc"
