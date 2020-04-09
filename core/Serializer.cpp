#include "Serializer.hpp"

#include <QObject>
namespace SSRPlugin
{
    const QJsonObject SSRSerializer::DeserializeOutbound(const QString &ssrBase64Uri, QString *alias, QString *errMessage) const
    {
        ShadowSocksRServerObject server;
        QString d_name;
        if (ssrBase64Uri.length() < 6)
        {
            *errMessage = QObject::tr("SSR URI is too short");
            return {};
        }
        QRegExp regex("^(.+):([^:]+):([^:]*):([^:]+):([^:]*):([^:]+)");
        auto data = SafeBase64Decode(ssrBase64Uri.mid(6));
        // bool matchSuccess = regex.exactMatch(ssrUrl);
        for (int nTry = 0; nTry < 2; ++nTry)
        {
            int param_start_pos = data.indexOf("?");
            if (param_start_pos > 0)
            {
                auto paramas = data.mid(param_start_pos + 1).split('&');
                for (auto &parama : paramas)
                {
                    auto pos = parama.indexOf('=');
                    auto key = parama.mid(0, pos);
                    auto val = parama.mid(pos + 1);
                    if (key == "obfsparam")
                    {
                        server.obfs_param = SafeBase64Decode(val);
                    }
                    else if (key == "protoparam")
                    {
                        server.protocol_param = SafeBase64Decode(val);
                    }
                    else if (key == "remarks")
                    {
                        server.remarks = SafeBase64Decode(val);
                        d_name = server.remarks;
                    }
                    else if (key == "group")
                    {
                        server.group = SafeBase64Decode(val);
                    }
                }
                // params_dict = ParseParam(data.mid(param_start_pos + 1));
                data = data.mid(0, param_start_pos);
            }
            if (data.indexOf("/") >= 0)
            {
                data = data.mid(0, data.lastIndexOf("/"));
            }

            auto matched = regex.exactMatch(data);
            auto list = regex.capturedTexts();
            if (matched && list.length() == 7)
            {
                server.address = list[1];
                server.port = list[2].toInt();
                server.protocol = list[3].length() == 0 ? "origin" : list[3];
                server.protocol = server.protocol.replace("_compatible", "");
                server.method = list[4];
                server.obfs = list[5].length() == 0 ? "plain" : list[5];
                server.password = SafeBase64Decode(list[6]);
                break;
            }
            *errMessage = QObject::tr("SSRUrl not matched regex \"^(.+):([^:]+):([^:]*):([^:]+):([^:]*):([^:]+)\"");
            return {};
        }
        QJsonObject root;
        QJsonArray outbounds;

        auto GenerateShadowSocksROUT = [](const QList<ShadowSocksRServerObject> &servers) {
            QJsonObject root;
            QJsonArray x;
            for (const auto &server : servers)
            {
                x.append(server.toJson());
            }
            root.insert("servers", x);
            return root;
        };

        auto GenerateOutboundEntry = [](const QString &protocol, const QJsonObject &settings) {
            QJsonObject root;
            root.insert("protocol", protocol);
            root.insert("settings", settings);
            return root;
        };

        outbounds.append(GenerateOutboundEntry("shadowsocksr", GenerateShadowSocksROUT(QList<ShadowSocksRServerObject>() << server)));
        root.insert("outbounds", outbounds);
        *alias =
            alias->isEmpty() ?
                d_name.isEmpty() ? server.address + server.port + server.group + server.protocol + server.method + server.password : d_name :
                *alias + "_" + d_name;
        *alias = alias->trimmed();
        if (alias->isEmpty())
        {
            *errMessage = QObject::tr("SSRUrl empty");
            return {};
        }
        // LOG(MODULE_CONNECTION, "Deduced alias: " + *alias)
        return root;
    }

    const QString SSRSerializer::SerializeOutbound(const QJsonObject &object) const
    {
        auto server = ShadowSocksRServerObject::fromJson(object);
        QString main_part = server.address + ":" + QString::number(server.port) + ":" + server.protocol + ":" + server.method + ":" +
                            server.obfs + ":" + SafeBase64Encode(server.password);
        QString param_str = "obfsparam=" + SafeBase64Encode(server.obfs_param);
        if (!server.protocol_param.isEmpty())
        {
            param_str += "&protoparam=" + SafeBase64Encode(server.protocol_param);
        }
        if (!server.remarks.isEmpty())
        {
            param_str += "&remarks=" + SafeBase64Encode(server.remarks);
        }
        if (!server.group.isEmpty())
        {
            param_str += "&group=" + SafeBase64Encode(server.group);
        }
        QString base64 = SafeBase64Encode(main_part + "/?" + param_str);
        return "ssr://" + base64;
    }
} // namespace SSRPlugin