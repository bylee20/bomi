#include <QtCore/QCoreApplication>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include "enumgenerator.hpp"

static void makeShader();

int main(/*int argc, char **argv*/) {
	EnumGenerator::generate2();
	makeShader();
	return 0;
}

void makeShader() {
	// cgc -profile arbfp1 i420_to_rgb_simple.cg | tail +3 | xxd -i > ../i420_to_rgb_simple.cgc
       QStringList files;
       files << "i420_to_rgb_simple.cg" << "i420_to_rgb_filter.cg" << "i420_to_rgb_kernel.cg";
       const QString cmd("cgc -profile arbfp1 %1 | tail +3 | xxd -i > ../%1c");
       for (int i=0; i<files.size(); ++i)
	       system(cmd.arg(files[i]).toLocal8Bit());
}
