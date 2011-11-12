#include "shaderinterpreter.hpp"
#include <QtCore/QDebug>

Interpreter::Interpreter() {
	op['+'] = "ADD";
	op['-'] = "SUB";
	op['*'] = "MUL";
	cmd.insert("ADD");
	cmd.insert("SUB");
	cmd.insert("MUL");
	cmd.insert("DP3");
	cmd.insert("TEX");
	cmd.insert("MOV");
	cmd.insert("ADD_SAT");
	cmd.insert("SUB_SAT");
	cmd.insert("MUL_SAT");
	cmd.insert("DP3_SAT");
	cmd.insert("TEX");
	cmd.insert("MOV_SAT");
	m_hasError = false;
//		m_errorLine = -1;
}

QList<QByteArray> Interpreter::split(const QByteArray &data, const char *sep) {
	const int len = qstrlen(sep);
	QList<QByteArray> ret;
	int from = -1;
	int count = 0;
	for (int i=0; i<data.size(); ++i) {
		const char c = data[i];
		bool isSep = false;
		for (int j=0; j<len; ++j) {
			if (c == sep[j]) {
				isSep = true;
				break;
			}
		}
		if (isSep) {
			if (from != -1) {
				const QByteArray token = data.mid(from, count).trimmed();
				if (!token.isEmpty())
					ret.push_back(token);
				from = -1;
				count = 0;
			}
		} else {
			if (from < 0)
				from = i;
			++count;
		}

	}
	if (from != -1) {
		const QByteArray token = data.mid(from, count).trimmed();
		if (!token.isEmpty())
			ret.push_back(token);
	}
	return ret;
}

bool Interpreter::interpret(const QByteArray &src) {
	reset();
	m_code = "!!ARBfp1.0\n";
	int cur = 0;
	int end = src.lastIndexOf('\0');
	if (end < 0)
		end = src.size();
	QByteArray inst;
	while (cur < end) {
		if (skipSeperator(cur, src))
			break;
		int idx = src.indexOf('\n', cur);
		if (idx < 0)
			idx = src.indexOf('\r', cur);
		if (idx < 0)
			idx = end;
		QByteArray line = src.mid(cur, idx-cur);
		cur = idx;
		const int cmt = line.indexOf("//");
		if (cmt >= 0)
			line = line.mid(0, cmt);
		QList<QByteArray> insts;
		if (!inst.isEmpty()) {
			idx = line.indexOf(';');
			if (idx < 0) {
				inst += line;
				continue;
			} else {
				inst += line.mid(0, idx+1);
				line = line.mid(idx+1, -1);
				insts.push_back(inst.trimmed());
				inst.clear();
			}
		}
		int from = 0;
		for (;;) {
			const int to = line.indexOf(';', from);
			if (to < 0) {
				inst = line.mid(from);
				break;
			}
			insts.push_back(line.mid(from, to-from+1).trimmed());
			from = to+1;
		}
		for (int i=0; i<insts.size(); ++i) {
			if (!parseInstruction(insts[i]))
				return false;
		}
	}
	inst = inst.trimmed();
	if (!inst.isEmpty() && !parseInstruction(inst))
		return false;
	m_code += "END\n";
	return true;
}

bool Interpreter::parseInstruction(const QByteArray &src) {
	m_errorInst = src;
	if (src.isEmpty())
		return true;
	if (src[src.size()-1] != ';') {
		setError("An instruction should be terminated with ';'.");
		return false;
	}
	const QByteArray token = src.left(src.size()-1);
	if (token.isEmpty())
		return true;
	if (parseDeclaration(token))
		return !m_hasError;
	if (parseAssignment(token))
		return !m_hasError;
	setError("Instruction should include an assignment or a declaration.");
	return false;
}

bool Interpreter::parseAssignment(const QByteArray &src) {
	const int assign = src.indexOf('=');
	if (assign < 0)
		return false;
	const QByteArray rhs = src.mid(assign + 1).trimmed();
	if (assign == 0)
		goto lhs_empty;
	if (rhs.isEmpty())
		goto rhs_empty;
	if (op.contains(src[assign-1])) {
		const QByteArray var = src.left(assign-1).trimmed();
		if (var.isEmpty())
			goto lhs_empty;
		const int p = var.indexOf('.');
		const QByteArray name = p < 0 ? var : var.mid(0, p);
		if (!m_var.contains(name))
			goto no_var;
		m_code += op[src[assign-1]];
		m_code += ' ';
		m_code += var;
		m_code += ", ";
		m_code += var;
		m_code += ", ";
		m_code += rhs;
		m_code += ";\n";
		return true;
	} else {
		const QByteArray var = src.left(assign).trimmed();
		if (var.isEmpty())
			goto lhs_empty;
		const int p = var.indexOf('.');
		const QByteArray name = p < 0 ? var : var.mid(0, p);
		if (!m_var.contains(name))
			goto no_var;
		const int op = indexOf(rhs, 0, "+-*", 3);
		QByteArray cmd;
		QList<QByteArray> param;
		if (op != -1) {
			cmd = this->op[rhs[op]];
			param.push_back(rhs.left(op));
			param.push_back(rhs.mid(op+1));
		} else {
			int left = rhs.indexOf('(');
			int right = rhs.indexOf(')');
			if (left >= 0 && right >= 0) {
				if (right < left)
					goto invalid_rhs_format;
				cmd = rhs.left(left).trimmed();
				param = rhs.mid(left+1, right-left-1).split(',');
			} else if (left < 0 && right < 0) {
				cmd = "MOV";
				param.push_back(rhs);
			} else
				goto invalid_rhs_format;
		}
		if (cmd.isEmpty() || param.isEmpty())
			goto invalid_rhs_format;
		if (!this->cmd.contains(cmd)) {
			setError("Command does not exsits.");
			return true;
		}
		m_code += cmd;
		m_code += ' ';
		m_code += var;
		for (int i=0; i<param.size(); ++i) {
			m_code += ", ";
			m_code += param[i].trimmed();
		}
		m_code += ";\n";
		return true;
	}
lhs_empty:
	setError("LHS for assignment is empty.");
	return true;
rhs_empty:
	setError("RHS for assignment is empty.");
	return true;
no_var:
	setError("Variable is not declared.");
	return true;
invalid_rhs_format:
	setError("Format of RHS for assignment is not valid.");
	return true;
}

bool Interpreter::parseDeclaration(const QByteArray &src) {
	int idx = indexOfSeperator(src);
	if (idx < 4 || idx > 6)
		return false;
	const QByteArray type = src.left(idx);
	if (type == "ATTRIB" || type == "OUTPUT" || type == "PARAM" || type == "ALIAS") {
		QList<QByteArray> list = src.split('=');
		if (list.size() != 2)
			goto no_var_init;
		const QByteArray lhs = list.at(0);
		const QByteArray rhs = list.at(1).trimmed();
		list = split(lhs, " \t\n\r");
		if (list.size() != 2)
			goto no_var_name;
		Q_ASSERT(type == list.at(0));
		VarInfo var;
		var.type = list.at(0);
		var.name = list.at(1);
		var.init = rhs;
		if (m_var.contains(var.name))
			goto var_exists;
		if (var.init.isEmpty())
			goto no_var_init;
		m_var.insert(var.name, var);
		m_code += src;
		m_code += ";\n";
		return true;
	} else if (type == "TEMP") {
		const QList<QByteArray> list = src.mid(idx+1).split(',');
		for (int i=0; i<list.size(); ++i) {
			VarInfo var;
			var.type = type;
			var.name = list[i].trimmed();
			if (m_var.contains(var.name))
				goto var_exists;
			m_var.insert(var.name, var);
		}
		m_code += src;
		m_code += ";\n";
		return true;
	} else
		return false;
no_var_name:
	setError("No variable name in declaration.");
	return true;
//	invalid_var_name:
//		setError("Variable name contains non-word-character.");
//		return true;
no_var_init:
	setError("Non-TEMP variable should be initialized.");
	return true;
var_exists:
	setError("Declared variable already exists.");
	return true;
}

