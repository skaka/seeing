///////////////////////////////////////////////////////////////////////////////
/// @file   dummy_ai_engine.cpp
/// @brief  Mock AI engine — parses simple text commands into JSON mutations.
///
/// Supported commands (case-insensitive):
///   • "add clip <name> on track <N> from <start>s to <end>s"
///   • "remove clip <name>"
///   • "trim clip <name> to <duration>s"
///   • "move clip <name> to <position>s"
///   • "cut the first <N> seconds"   (trims first clip)
///   • "add demo"                    (adds a preset demo clip)
///
/// Any unrecognized input returns an "unknown" action with a help message.
///////////////////////////////////////////////////////////////////////////////

#include "dummy_ai_engine.h"

#include <QRegularExpression>
#include <QJsonArray>
#include <QUuid>

namespace Seeing {

DummyAiEngine::DummyAiEngine(QObject* parent)
    : AiEngineInterface(parent)
{
}

QString DummyAiEngine::engineName() const
{
    return QStringLiteral("Dummy AI (MVP)");
}

bool DummyAiEngine::isCloudBased() const
{
    return false;
}

QJsonObject DummyAiEngine::processPrompt(const QString& prompt,
                                          const QJsonObject& projectState)
{
    Q_UNUSED(projectState);

    QString input = prompt.trimmed().toLower();

    // ── "add demo" shortcut ─────────────────────────────────────────────────
    if (input == "add demo" || input == "demo") {
        QJsonObject cmd;
        cmd["action"] = "add_clip";
        QJsonObject params;
        params["id"]             = QUuid::createUuid().toString(QUuid::WithoutBraces);
        params["name"]           = "Demo Clip";
        params["track_index"]    = 0;
        params["timeline_start"] = 0.0;
        params["timeline_end"]   = 10.0;
        params["media_in"]       = 0.0;
        params["media_out"]      = 10.0;
        params["color"]          = "#4fc3f7";
        cmd["params"] = params;
        emit statusMessage("✅ Adding a demo clip (0s → 10s on Track 1).");
        return cmd;
    }

    // ── "add clip ..." ──────────────────────────────────────────────────────
    {
        QRegularExpression rx(
            R"(add\s+clip\s+(\w+)\s+on\s+track\s+(\d+)\s+from\s+([\d.]+)s?\s+to\s+([\d.]+)s?)",
            QRegularExpression::CaseInsensitiveOption);
        auto match = rx.match(input);
        if (match.hasMatch()) {
            QJsonObject cmd;
            cmd["action"] = "add_clip";
            QJsonObject params;
            params["id"]             = QUuid::createUuid().toString(QUuid::WithoutBraces);
            params["name"]           = match.captured(1);
            params["track_index"]    = match.captured(2).toInt();
            params["timeline_start"] = match.captured(3).toDouble();
            params["timeline_end"]   = match.captured(4).toDouble();
            params["media_in"]       = match.captured(3).toDouble();
            params["media_out"]      = match.captured(4).toDouble();
            params["color"]          = "#81c784";
            cmd["params"] = params;
            emit statusMessage(
                QString("✅ Adding clip '%1' on Track %2 (%3s → %4s).")
                    .arg(match.captured(1), match.captured(2),
                         match.captured(3), match.captured(4)));
            return cmd;
        }
    }

    // ── "remove clip ..." ───────────────────────────────────────────────────
    {
        QRegularExpression rx(R"(remove\s+clip\s+(\w+))",
                              QRegularExpression::CaseInsensitiveOption);
        auto match = rx.match(input);
        if (match.hasMatch()) {
            QJsonObject cmd;
            cmd["action"] = "remove_clip";
            QJsonObject params;
            params["name"] = match.captured(1);
            cmd["params"]  = params;
            emit statusMessage(
                QString("🗑️ Removing clip '%1'.").arg(match.captured(1)));
            return cmd;
        }
    }

    // ── "cut the first N seconds" ───────────────────────────────────────────
    {
        QRegularExpression rx(R"(cut\s+(?:the\s+)?first\s+([\d.]+)\s*s(?:econds?)?)",
                              QRegularExpression::CaseInsensitiveOption);
        auto match = rx.match(input);
        if (match.hasMatch()) {
            double cutDuration = match.captured(1).toDouble();
            QJsonObject cmd;
            cmd["action"] = "trim_first_clip";
            QJsonObject params;
            params["cut_seconds"] = cutDuration;
            cmd["params"]         = params;
            emit statusMessage(
                QString("✂️ Cutting the first %1 seconds from the first clip.")
                    .arg(cutDuration));
            return cmd;
        }
    }

    // ── "trim clip <name> to <duration>s" ───────────────────────────────────
    {
        QRegularExpression rx(R"(trim\s+clip\s+(\w+)\s+to\s+([\d.]+)s?)",
                              QRegularExpression::CaseInsensitiveOption);
        auto match = rx.match(input);
        if (match.hasMatch()) {
            QJsonObject cmd;
            cmd["action"] = "trim_clip";
            QJsonObject params;
            params["name"]     = match.captured(1);
            params["duration"] = match.captured(2).toDouble();
            cmd["params"]      = params;
            emit statusMessage(
                QString("✂️ Trimming clip '%1' to %2s.")
                    .arg(match.captured(1), match.captured(2)));
            return cmd;
        }
    }

    // ── "move clip <name> to <position>s" ───────────────────────────────────
    {
        QRegularExpression rx(R"(move\s+clip\s+(\w+)\s+to\s+([\d.]+)s?)",
                              QRegularExpression::CaseInsensitiveOption);
        auto match = rx.match(input);
        if (match.hasMatch()) {
            QJsonObject cmd;
            cmd["action"] = "move_clip";
            QJsonObject params;
            params["name"]     = match.captured(1);
            params["position"] = match.captured(2).toDouble();
            cmd["params"]      = params;
            emit statusMessage(
                QString("➡️ Moving clip '%1' to %2s.")
                    .arg(match.captured(1), match.captured(2)));
            return cmd;
        }
    }

    // ── "undo" / "redo" ─────────────────────────────────────────────────────
    if (input == "undo") {
        QJsonObject cmd;
        cmd["action"] = "undo";
        emit statusMessage("↩️ Undoing last action.");
        return cmd;
    }
    if (input == "redo") {
        QJsonObject cmd;
        cmd["action"] = "redo";
        emit statusMessage("↪️ Redoing last action.");
        return cmd;
    }

    // ── "help" ──────────────────────────────────────────────────────────────
    if (input == "help" || input == "?") {
        QJsonObject cmd;
        cmd["action"] = "help";
        emit statusMessage(
            "📋 Available commands:\n"
            "  • add demo\n"
            "  • add clip <name> on track <N> from <start>s to <end>s\n"
            "  • remove clip <name>\n"
            "  • trim clip <name> to <duration>s\n"
            "  • move clip <name> to <position>s\n"
            "  • cut the first <N> seconds\n"
            "  • undo / redo\n"
            "  • help");
        return cmd;
    }

    // ── Fallback ────────────────────────────────────────────────────────────
    QJsonObject cmd;
    cmd["action"] = "unknown";
    cmd["error"]  = "Unrecognized command. Type 'help' for available commands.";
    emit statusMessage("⚠️ I didn't understand that. Type 'help' for a list of commands.");
    return cmd;
}

} // namespace Seeing
