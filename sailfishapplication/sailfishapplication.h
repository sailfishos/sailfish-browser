
#ifndef SAILFISHAPPLICATION_H
#define SAILFISHAPPLICATION_H

class QString;
class QApplication;
class QDeclarativeView;

namespace Sailfish {

QApplication *createApplication(int &argc, char **argv);
QDeclarativeView *createView();
void showView(QDeclarativeView * view);

}

#endif // SAILFISHAPPLICATION_H

