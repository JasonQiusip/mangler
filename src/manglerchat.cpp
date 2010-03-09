/*
 * vim: softtabstop=4 shiftwidth=4 cindent foldmethod=marker expandtab
 *
 * $LastChangedDate$
 * $Revision$
 * $LastChangedBy$
 * $URL$
 *
 * Copyright 2009-2010 Eric Kilfoil
 *
 * This file is part of Mangler.
 *
 * Mangler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mangler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mangler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mangler.h"
#include "manglerchat.h"
#include "manglersettings.h"
#include "manglerconfig.h"

#include "manglercharset.h"

ManglerChat::ManglerChat(Glib::RefPtr<Gtk::Builder> builder) {/*{{{*/
    this->builder = builder;

    builder->get_widget("chatWindow", chatWindow);
    chatWindow->signal_show().connect(sigc::mem_fun(this, &ManglerChat::chatWindow_show_cb));
    chatWindow->signal_hide().connect(sigc::mem_fun(this, &ManglerChat::chatWindow_hide_cb));

    builder->get_widget("sendChat", button);
    button->signal_clicked().connect(sigc::mem_fun(this, &ManglerChat::chatWindowSendChat_clicked_cb));

    builder->get_widget("chatClose", button);
    button->signal_clicked().connect(sigc::mem_fun(this, &ManglerChat::chatClose_clicked_cb));

    builder->get_widget("chatHide", button);
    button->signal_clicked().connect(sigc::mem_fun(this, &ManglerChat::chatHide_clicked_cb));

    builder->get_widget("chatMessage", chatMessage);
    chatMessage->set_activates_default(true);

    builder->get_widget("chatUserListTreeView", chatUserListView);
    chatUserTreeModel = Gtk::ListStore::create(chatUserColumns);
    chatUserTreeModelFilter = Gtk::TreeModelFilter::create(chatUserTreeModel);
    chatUserTreeModelFilter->set_visible_func( sigc::mem_fun(*this, &ManglerChat::filterVisible) );
    chatUserListView->set_model(chatUserTreeModelFilter);

    chatUserListView->append_column("Name", chatUserColumns.name);

    builder->get_widget("chatTimestampCheckButton", checkbutton);
    checkbutton->signal_toggled().connect(sigc::mem_fun(this, &ManglerChat::chatTimestampCheckButton_toggled_cb));

    builder->get_widget("chatBox", chatBox);
    isOpen = false;
    isJoined = false;
}/*}}}*/

void ManglerChat::chatTimestampCheckButton_toggled_cb(void) {/*{{{*/
    builder->get_widget("chatTimestampCheckButton", checkbutton);
    Mangler::config["ChatTimestamps"] = checkbutton->get_active();
    Mangler::config.config.save();
}/*}}}*/

void ManglerChat::chatWindow_show_cb(void) {/*{{{*/
    isOpen = true;
    if (v3_is_loggedin() && !isJoined) {
        v3_join_chat();
        isJoined = true;
    }
    builder->get_widget("chatTimestampCheckButton", checkbutton);
    checkbutton->set_active(Mangler::config["ChatTimestamps"].toBool());
}/*}}}*/

void ManglerChat::chatWindow_hide_cb(void) {/*{{{*/
    isOpen = false;
}/*}}}*/

void ManglerChat::chatWindowSendChat_clicked_cb(void) {/*{{{*/
    if (chatMessage->get_text_length()) {
        v3_send_chat_message((char *)ustring_to_c(chatMessage->get_text()).c_str());
        chatMessage->set_text("");
    }
}/*}}}*/

void ManglerChat::chatClose_clicked_cb(void) {/*{{{*/
    if (v3_is_loggedin()) {
        v3_leave_chat();
    }
    isJoined = false;
    chatWindow->hide();
}/*}}}*/

void ManglerChat::chatHide_clicked_cb(void) {/*{{{*/
    chatWindow->hide();
}/*}}}*/

void ManglerChat::addMessage(Glib::ustring message) {/*{{{*/
    char timestamp[200];
    time_t t;
    struct tm *tmp;
    struct timeval tv;

    Glib::RefPtr<Gtk::TextBuffer> buffer = chatBox->get_buffer();
    if(Mangler::config["ChatTimestamps"].toBool()) {
        gettimeofday(&tv, NULL);
        t = tv.tv_sec;
        tmp = localtime(&t);
        if (strftime(timestamp, sizeof(timestamp), "%T", tmp) != 0) {
            message = "[" + (Glib::ustring)timestamp + "] " + message;
        }
    }
    buffer->set_text(buffer->get_text() + message + "\n");

    Gtk::TextIter end = buffer->end();
    Glib::RefPtr<Gtk::TextMark> end_mark = buffer->create_mark(end);
    chatBox->scroll_to(end_mark, 0.0);
}/*}}}*/

Glib::ustring ManglerChat::nameFromId(uint16_t user_id) {/*{{{*/
    v3_user *u;
    Glib::ustring name = "";

    if ((u = v3_get_user(user_id))) {
        name = c_to_ustring(u->name);
        v3_free_user(u);
    }

    return name;
}/*}}}*/

void ManglerChat::addChatMessage(uint16_t user_id, Glib::ustring message) {/*{{{*/
    addMessage("[" + nameFromId(user_id) + "]: " + message);
}/*}}}*/

void ManglerChat::addRconMessage(Glib::ustring message) {/*{{{*/
    addMessage("[RCON]: " + message);
}/*}}}*/

void ManglerChat::addUser(uint16_t user_id) {/*{{{*/
    if (isUserInChat(user_id)) {
        return;
    }
    Glib::ustring name = nameFromId(user_id);
    if (name == "") {
        return;
    }
    chatUserIter = chatUserTreeModel->append();
    chatUserRow = *chatUserIter;
    chatUserRow[chatUserColumns.id] = user_id;
    chatUserRow[chatUserColumns.name] = name;
    chatUserRow[chatUserColumns.channel] = v3_get_user_channel(user_id);
    chatUserTreeModelFilter->refilter();
    if (isGlobal || (v3_get_user_channel(user_id) == v3_get_user_channel(mangler->myID)) ) {
        addMessage("* " + name + " has joined the chat.");
    }
}/*}}}*/

void ManglerChat::removeUser(uint16_t user_id) {/*{{{*/
    Gtk::TreeModel::Row row;
    Gtk::TreeModel::Children::iterator iter = chatUserTreeModel->children().begin();

    while (iter != chatUserTreeModel->children().end()) {
        row = *iter;
        uint32_t rowId = row[chatUserColumns.id];
        if (rowId == user_id) {
            if (isGlobal || (v3_get_user_channel(user_id) == v3_get_user_channel(mangler->myID)) ) {
                addMessage("* " + row[chatUserColumns.name] + " has left the chat.");
            }
            chatUserTreeModel->erase(row);
            chatUserTreeModelFilter->refilter();
            return;
        }
        iter++;
    }
    return;
}/*}}}*/

bool ManglerChat::filterVisible(const Gtk::TreeIter& iter) {/*{{{*/
    uint16_t theirChannel = (*iter)[chatUserColumns.channel];
    if (((*iter)[chatUserColumns.id] == mangler->myID) || isGlobal || (theirChannel == v3_get_user_channel(mangler->myID))) {
        return true;
    }
    return false;
}/*}}}*/

void ManglerChat::clear(void) {/*{{{*/
    chatUserTreeModel->clear();
    addMessage("*** disconnected from server");
    //chatBox->get_buffer()->set_text("");
    isJoined = false;
}/*}}}*/

bool ManglerChat::isUserInChat(uint16_t user_id) {/*{{{*/
    Gtk::TreeModel::Row row;
    Gtk::TreeModel::Children::iterator iter = chatUserTreeModel->children().begin();

    while (iter != chatUserTreeModel->children().end()) {
        row = *iter;
        uint32_t rowId = row[chatUserColumns.id];
        if (rowId == user_id) {
            return true;
        }
        iter++;
    }
    return false;
}/*}}}*/

void ManglerChat::updateUser(uint16_t user_id) {/*{{{*/
    Gtk::TreeModel::Children::iterator iter = chatUserTreeModel->children().begin();
    while (iter != chatUserTreeModel->children().end()) {
        if ((*iter)[chatUserColumns.id] == user_id) {
            (*iter)[chatUserColumns.channel] = v3_get_user_channel(user_id);
            return;
        }
        iter++;
    }
    return;
}/*}}}*/

