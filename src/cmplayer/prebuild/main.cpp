#include <QtCore/QCoreApplication>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include "shaderinterpreter.hpp"
#include "enumgenerator.hpp"

static void makeShader();

int main(int argc, char **argv) {
	EnumGenerator::generate2();
//	qDebug() << Enum::__ENUM_CLASS::Item0.name();
//	qDebug() << ListEnum::Item0.id() << ListEnum::Item0.toString();
//	Test t = Test::v1;
//	Test::v1.key();
	QCoreApplication app(argc, argv);
	makeShader();
//	EnumGenerator::generate();
	return 0;
}

void makeShader() {
       QStringList files;
       files << "i420_to_rgb_simple.cpp" << "i420_to_rgb_filter.cpp" << "i420_to_rgb_kernel.cpp";
       for (int i=0; i<files.size(); ++i) {
	       qDebug() << "Interpret" << files[i];
	       QProcess proc;
	       proc.setReadChannel(QProcess::StandardOutput);
	       QStringList args;
	       args << "-E" << files[i];
	       proc.start("g++", args, QProcess::ReadOnly);
	       const bool ready = proc.waitForReadyRead();
	       Q_ASSERT(ready);
	       QList<QByteArray> lines = proc.readAllStandardOutput().split('\n');
	       QByteArray output;
	       for (int j=0; j<lines.size(); ++j) {
		       if (!lines[j].startsWith('#')) {
			       output += lines[j];
			       output += '\n';
		       }
	       }

	       if (!proc.waitForFinished())
		       proc.kill();
	       Interpreter interpreter;
	       if (!interpreter.interpret(output))
		       qFatal("Interpreter Error: %s", qPrintable(interpreter.errorMessage()));
	       const QFileInfo info(files[i]);
	       const QString base = info.completeBaseName();
	       QFile fp(base + ".fp");
	       fp.open(QFile::WriteOnly | QFile::Truncate);
	       Q_ASSERT(fp.isOpen());
	       fp.write(interpreter.code());
	       fp.close();
	       args.clear();
	       args << "-i" << (base + ".fp") << ("../" + base + ".hpp");
	       proc.start("xxd", args, QProcess::ReadOnly);
	       if (!proc.waitForFinished())
		       proc.kill();
       }
}
