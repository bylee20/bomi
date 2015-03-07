#include "jrplayer.hpp"
#include "quick/appobject.hpp"
#include "json/jrcommon.hpp"
#include "misc/jsonstorage.hpp"

struct JrPlayer::Data {
    AppObject app;
    QMetaObject *mo = nullptr;
    PlayEngine *engine;
    PlaylistModel *playlist;
    HistoryModel *history;
    WindowObject *window;

    using ParamArray = std::array<QVariant, 10>;

    auto invoke(QObject *object, const QMetaMethod &method, const QList<QVariant> &params) -> QJsonValue
    {
        if (method.parameterCount() > 10)
            return QJsonValue::Undefined; // cannot call with over 10 params
        Q_ASSERT(method.parameterCount() == params.size());

        std::array<QGenericArgument, 10> args;
        for (int i = 0; i < method.parameterCount(); ++i)
            args[i] = QGenericArgument(params[i].typeName(), params[i].constData());
        auto ret = _QVariantFromType(method.returnType());
        QGenericReturnArgument rarg;
        if (ret.userType() != QMetaType::Void)
            rarg = QGenericReturnArgument(ret.typeName(), ret.data());
        if (!method.invoke(object, rarg, args[0], args[1], args[2], args[3], args[4],
                          args[5], args[6], args[7], args[8], args[9]))
            return QJsonValue::Undefined;
        if (ret.userType() == QMetaType::Void)
            return QJsonValue::Null;
        const auto qobject = ret.value<QObject*>();
        if (qobject)
            return _JsonFromQObject(qobject);
        return _JsonFromQVariant(ret);
    }

    auto invoke(QObject *object, const QMetaMethod &method, const QJsonArray &array) -> QJsonValue
    {
        if (method.parameterCount() != array.size())
            return QJsonValue::Undefined;
        QList<QVariant> params;
        params.reserve(array.size());
        for (int i = 0; i < method.parameterCount(); ++i) {
            auto param = _JsonToQVariant(array.at(i), method.parameterType(i));
            if (!param.isValid())
                return QJsonValue::Undefined;
            params.push_back(param);
        }
        return invoke(object, method, params);
    }

    auto invoke(QObject *object, const QMetaMethod &method, const QJsonObject &json) -> QJsonValue
    {
        if (method.parameterCount() != json.size())
            return QJsonValue::Undefined;
        QList<QVariant> params;
        params.reserve(json.size());
        const auto names = method.parameterNames();
        Q_ASSERT(names.size() == method.parameterCount());
        for (int i = 0; i < method.parameterCount(); ++i) {
            auto param = _JsonToQVariant(json[_L(names[i])], method.parameterType(i));
            if (!param.isValid())
                return QJsonValue::Undefined;
            params.push_back(params);
        }
        return invoke(object, method, params);
    }
};

JrPlayer::JrPlayer(QObject *parent)
    : JrIface(parent), d(new Data)
{
}

JrPlayer::~JrPlayer()
{
    delete d;
}

auto JrPlayer::request(const JrRequest &request) -> JrResponse
{
    Q_ASSERT(request.isValid());
    QObject *object = &d->app;
    int pos = 0;
    const auto jrMethod = request.method();
    const auto jrParams = request.params();
    if (jrMethod.startsWith("App."_a))
        pos = 4;
    auto error = [&] (JrError e) { return _JrErrorResponse(request.id(), e); };
    while (pos < jrMethod.size()) {
        const int next = jrMethod.indexOf('.'_q, pos);
        if (next > pos) {
            const auto name = jrMethod.midRef(pos, next - pos).toUtf8();
            pos = next + 1;
            const int left = name.indexOf('[');
            if (left > 0) {
                QQmlListReference list(object, name.left(left));
                const int right = name.indexOf(']', left);
                if (!list.isValid() || right < 0)
                    break;
                bool ok = false;
                const int idx = name.mid(left + 1, right - (left + 1)).toInt(&ok);
                const auto obj = list.at(idx);
                if (!ok || !obj)
                    break;
                object = obj;
                continue;
            }
            const auto p = object->property(name);
            if (auto obj = p.value<QObject*>()) {
                object = obj;
                continue;
            }
            QQmlListReference list(object, name);
            if (!list.isValid() || jrMethod.midRef(pos) != "length"_a)
                break;
            return { request, list.count() };
        }
        const auto name = jrMethod.midRef(pos).toUtf8();
        pos = jrMethod.size();
        if (name.isEmpty())
            break;

        const auto mo = object->metaObject();
        const int idx = mo->indexOfProperty(name);
        if (idx >= 0) {
            const auto p = mo->property(idx);
            if (!jrParams.isUndefined()) {
                QJsonValue value(QJsonValue::Undefined);
                if (jrParams.isArray()) {
                    auto array = jrParams.toArray();
                    if (array.size() != 1)
                        return error(JrError::InvalidParams);
                    value = array.at(0);
                } else if (jrParams.isObject()) {
                    auto object = jrParams.toObject();
                    if (object.size() != 1)
                        return error(JrError::InvalidParams);
                    value = object.begin().value();
                }
                if (value.isUndefined())
                    return error(JrError::InvalidParams);
                auto var = _JsonToQVariant(value, p.userType());
                if (!var.isValid())
                    return error(JrError::InvalidParams);
                if (!p.write(object, var))
                    return error(JrError::MethodNotFound);
            }
            const auto res = _JsonFromQVariant(p.read(object));
            if (!res.isUndefined())
                return { request, res };
            return _JrErrorResponse(request.id(), JrError::InternalError);
        }

        for (int i = 0; i < mo->methodCount(); ++i) {
            if (mo->method(i).name() != name)
                continue;
            QJsonValue res(QJsonValue::Undefined);
            if (jrParams.isArray())
                res = d->invoke(object, mo->method(i), jrParams.toArray());
            else if (jrParams.isObject())
                res = d->invoke(object, mo->method(i), jrParams.toObject());
            else if (jrParams.isUndefined())
                res = d->invoke(object, mo->method(i), QJsonArray());
            if (!res.isUndefined())
                return { request, res };
        }
        return _JrErrorResponse(request.id(), JrError::InvalidParams);
    }
    return _JrErrorResponse(request.id(), JrError::MethodNotFound);
}
