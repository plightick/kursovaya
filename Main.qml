import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Window {  // можно оставить Window — без padding в корне
    width: 800
    height: 600
    visible: true
    title: qsTr("Банк")
    Material.theme: Material.Light
    Material.accent: Material.Teal

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: bank.authenticated ? mainPage : loginPage
    }

    Component {
        id: loginPage
        Item {
            width: stack.width
            height: stack.height
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12

                Label { text: "Вход"; font.pixelSize: 22; Layout.alignment: Qt.AlignHCenter }
                TextField { id: user; placeholderText: "Имя пользователя"; Layout.preferredWidth: 320; Layout.minimumWidth: 320 }
                TextField { id: pass; placeholderText: "Пароль"; echoMode: TextInput.Password; Layout.preferredWidth: 320; Layout.minimumWidth: 320 }
                RowLayout {
                    spacing: 8
                    Button { text: "Войти"; onClicked: bank.login(user.text, pass.text) }
                    Button { text: "Регистрация"; onClicked: bank.registerUser(user.text, pass.text) }
                }
                Label { id: status; text: ""; color: "#666" }
            }
            Connections {
                target: bank
                function onAuthenticatedChanged() { stack.replace(mainPage) }
                function onErrorOccured(message) { status.text = message }
                function onInfoMessage(message) {
                    // Не показываем сообщения об успешном входе, так как это уже отображается внизу
                    if (!message.includes("Вход выполнен")) status.text = message
                }
            }
        }
    }

    Component {
        id: mainPage
        Item {
            property var receiptData: ({})
            property string cancelTxId: ""
            property string adminCancelTxId: ""
            width: stack.width
            height: stack.height
            
            // Models
            ListModel { id: accModel }
            ListModel { id: recipientCardsModel }
            ListModel { id: recipientAccountsModel }
            ListModel { id: expenseListModel }
            
            // Functions
            function accRefresh() {
                accModel.clear()
                const a = bank.listAccounts()
                for (let i=0;i<a.length;i++) accModel.append({ text: a[i].accountNumber + " (" + a[i].currency + ")", value: a[i].accountNumber })
            }
            function favRefresh() {
                if (typeof favoritesList !== 'undefined') favoritesList.model = bank.listFavorites()
            }
            function checkCardExpiry(expiry) {
                // Функция использует исключения внутри C++ кода
                // CardExpiredError обрабатывается и возвращает true
                // ValidationError обрабатывается и возвращает false
                return bank.isCardExpired(expiry)
            }
            function computeStatement() {
                if (accModel.count === 0 || stmtAcc.currentIndex < 0) { stmtResult.text = "Выберите счет"; stmtResult.color = "#666"; return }
                const accounts = bank.listAccounts()
                const accNum = accModel.get(stmtAcc.currentIndex).value
                let current = 0
                for (let i=0;i<accounts.length;i++) {
                    if (accounts[i].accountNumber === accNum) {
                        current = accounts[i].balanceCents
                        break
                    }
                }
                const delta = current/100 - stmtInitial.value
                const sign = delta > 0 ? "+" : ""
                stmtResult.color = delta < 0 ? "#666" : "#18a558"
                stmtResult.text = "Изменение: " + sign + delta.toFixed(2)
            }
            function refreshRecipientCards() {
                recipientCardsModel.clear()
                const name = (typeof recipient !== 'undefined' && recipient.text) ? recipient.text : (typeof favRecipient !== 'undefined' ? favRecipient.text : "")
                const list = bank.listUserCards(name)
                for (let i=0;i<list.length;i++) recipientCardsModel.append({ text: list[i].cardNumber + " (" + list[i].holderName + ")", value: list[i].cardNumber })
                if (recipientCardsModel.count === 0) transferStatus.text = "Карты не найдены у пользователя"; else transferStatus.text = "Выберите карту получателя"
            }
            function refreshRecipientAccounts() {
                recipientAccountsModel.clear()
                const name = (typeof recipient !== 'undefined' && recipient.text) ? recipient.text : (typeof favRecipient !== 'undefined' ? favRecipient.text : "")
                const list = bank.listUserAccounts(name)
                for (let i=0;i<list.length;i++) recipientAccountsModel.append({ text: list[i].accountNumber + " (" + list[i].currency + ")", value: list[i].accountNumber })
            }
            function showReceipt(txId) {
                const receipt = bank.receiptFor(txId)
                if (receipt && receipt.id) {
                    receiptData = receipt
                    if (!receiptData.user) receiptData.user = bank.username
                    receiptDialog.open()
                } else {
                    transferStatus.text = "Чек недоступен"
                }
            }
            function downloadReceipt() {
                if (!receiptData || !receiptData.id) return
                receiptFileDialog.open()
            }
            function updateExpenseChart() {
                if (typeof expenseListModel === 'undefined') {
                    return
                }
                
                expenseListModel.clear()
                const stats = bank.getExpenseStats()
                if (!stats) {
                    if (typeof expenseChart !== 'undefined') {
                        expenseChart.requestPaint()
                    }
                    return
                }
                
                const colors = {
                    "medicine": "#4CAF50",
                    "sport": "#2196F3",
                    "food": "#FF9800",
                    "entertainment": "#9C27B0",
                    "other": "#607D8B"
                }
                
                const categories = ["medicine", "sport", "food", "entertainment", "other"]
                const categoryNames = {
                    "medicine": "Медицина и здравоохранение",
                    "sport": "Спорт",
                    "food": "Продукты",
                    "entertainment": "Развлечения",
                    "other": "Остальное"
                }
                
                for (let i = 0; i < categories.length; i++) {
                    const cat = categories[i]
                    const catData = stats[cat]
                    
                    if (catData !== undefined && catData !== null) {
                        let amount = 0
                        let percent = 0
                        let name = categoryNames[cat] || cat
                        
                        if (catData.amount !== undefined) {
                            amount = Number(catData.amount) || 0
                        }
                        if (catData.percent !== undefined) {
                            percent = Number(catData.percent) || 0
                        }
                        if (catData.name !== undefined) {
                            name = String(catData.name)
                        }
                        
                        if (amount > 0) {
                            expenseListModel.append({
                                name: name,
                                amount: amount,
                                percent: percent,
                                color: colors[cat]
                            })
                        }
                    }
                }
                
                // Обновляем диаграмму после обновления данных
                if (typeof expenseChart !== 'undefined') {
                    expenseChart.requestPaint()
                }
            }
            function openCancelDialog(txId) {
                cancelTxId = txId
                if (typeof cancelReason !== 'undefined') cancelReason.text = ""
                cancelDialog.open()
            }
            function submitCancel() {
                if (!cancelTxId) return
                bank.cancelTransfer(cancelTxId, cancelReason.text)
                cancelTxId = ""
            }
            
            RowLayout {
                anchors.fill: parent
                spacing: 0

                // Sidebar
                Frame {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 200
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            visible: !bank.admin
                            ColumnLayout {
                                width: parent.width
                                spacing: 8
                                Label { 
                                    text: "Меню"; 
                                    font.bold: true; 
                                    font.pixelSize: 18; 
                                    Layout.fillWidth: true
                                    Layout.bottomMargin: 8
                                }
                                Button { 
                                    Layout.fillWidth: true; 
                                    Layout.preferredHeight: 50; 
                                    font.pixelSize: 15; 
                                    text: "Счета"; 
                                    visible: !bank.admin; 
                                    onClicked: contentView.currentIndex = 0 
                                }
                                Button { 
                                    Layout.fillWidth: true; 
                                    Layout.preferredHeight: 50; 
                                    font.pixelSize: 15; 
                                    text: "Карты"; 
                                    visible: !bank.admin; 
                                    onClicked: contentView.currentIndex = 1 
                                }
                                Button { 
                                    Layout.fillWidth: true; 
                                    Layout.preferredHeight: 50; 
                                    font.pixelSize: 15; 
                                    text: "Переводы"; 
                                    visible: !bank.admin; 
                                    onClicked: contentView.currentIndex = 2 
                                }
                                Button { 
                                    Layout.fillWidth: true; 
                                    Layout.preferredHeight: 50; 
                                    font.pixelSize: 15; 
                                    text: "Избранные"; 
                                    visible: !bank.admin; 
                                    onClicked: contentView.currentIndex = 3 
                                }
                                Button { 
                                    Layout.fillWidth: true; 
                                    Layout.preferredHeight: 50; 
                                    font.pixelSize: 15; 
                                    text: "Выписка"; 
                                    visible: !bank.admin; 
                                    onClicked: {
                                        contentView.currentIndex = 4
                                        if (typeof updateExpenseChart !== 'undefined') updateExpenseChart()
                                    } 
                                }
                                Button { 
                                    Layout.fillWidth: true; 
                                    Layout.preferredHeight: 50; 
                                    font.pixelSize: 15; 
                                    text: "Уведомления"; 
                                    visible: !bank.admin; 
                                    onClicked: contentView.currentIndex = 5 
                                }
                            }
                        }
                        // Для админа - без ScrollView
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                            spacing: 8
                            visible: bank.admin
                            Label { 
                                text: "Меню"; 
                                font.bold: true; 
                                font.pixelSize: 18; 
                                Layout.fillWidth: true
                                Layout.bottomMargin: 8
                            }
                            Button { 
                                Layout.fillWidth: true; 
                                Layout.preferredHeight: 50; 
                                font.pixelSize: 15; 
                                text: "Платежи"; 
                                onClicked: {
                                    contentView.currentIndex = 6
                                    if (typeof adminTransfersList !== 'undefined') adminTransfersList.model = bank.listAllTransfers("")
                                } 
                            }
                            Button { 
                                Layout.fillWidth: true; 
                                Layout.preferredHeight: 50; 
                                font.pixelSize: 15; 
                                text: "Пользователи"; 
                                onClicked: {
                                    contentView.currentIndex = 7
                                    if (typeof adminUsersList !== 'undefined') adminUsersList.model = bank.getAllUsersInfo(adminUsersSort.currentText)
                                } 
                            }
                        }
                        Item { Layout.fillHeight: true }
                        Button { 
                            Layout.fillWidth: true; 
                            Layout.preferredHeight: 50; 
                            font.pixelSize: 15; 
                            text: "Выход"; 
                            onClicked: bank.logout() 
                        }
                        Label { 
                            id: authStatus
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            text: bank.authenticated ? "Вход:\n" + bank.username : "" 
                        }
                    }
                }

                // Content
                StackLayout {
                    id: contentView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onCurrentIndexChanged: {
                        if (currentIndex === 4 && !bank.admin && typeof updateExpenseChart !== 'undefined') {
                            updateExpenseChart()
                        }
                    }

                    // Все страницы — просто Item с Layout, без padding

                    // Accounts
                    Item {
                        ColumnLayout {
                            visible: !bank.admin
                            anchors.fill: parent
                            anchors.margins: 12  // ✅ безопасно!
                            spacing: 8
                            RowLayout {
                                spacing: 8
                                Button { text: "Добавить счет (RUB)"; onClicked: bank.addAccount("RUB") }
                                Button { text: "Обновить"; onClicked: accountsList.model = bank.listAccounts() }
                                Button { text: "Пополнить счет"; onClicked: {
                                        accRefresh()
                                        if (accModel.count === 0) { accStatus.text = "Сначала создайте счет"; return }
                                        if (depositAccount.currentIndex < 0 && accModel.count > 0) depositAccount.currentIndex = 0
                                        depositExternal.text = ""
                                        depositAmount.value = 1000
                                        depositDialog.open()
                                    } }
                            }
                            ListView {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                id: accountsList
                                model: bank.listAccounts()
                                spacing: 8
                                delegate: Frame {
                                    width: ListView.view.width
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4
                                        RowLayout {
                                            spacing: 12
                                            Label { text: "Счет: " + modelData.accountNumber; font.bold: true }
                                            Label { text: "Валюта: " + modelData.currency }
                                            Label { text: "Баланс: " + (modelData.balanceCents/100).toFixed(2) }
                                        }
                                    }
                                }
                            }
                            Label { id: accStatus; text: ""; color: "#666" }
                        }
                    }

                    // Cards (current user)
                    Item {
                        ColumnLayout {
                            visible: !bank.admin
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            Label { text: "Добавление карты"; font.bold: true }
                            RowLayout {
                                spacing: 6
                                TextField { id: cardHolder; placeholderText: "Владелец"; text: bank.username; readOnly: true; Layout.preferredWidth: 240; Layout.minimumWidth: 200 }
                                TextField { id: cardExpiry; placeholderText: "Срок (MM/YY)"; Layout.preferredWidth: 140 }
                                ComboBox {
                                    id: accCombo
                                    Layout.preferredWidth: 400
                                    Layout.minimumWidth: 300
                                    Layout.fillWidth: true
                                    textRole: "text"
                                    model: accModel
                                    popup.width: Math.max(400, implicitContentWidth + 40)
                                    implicitContentWidthPolicy: ComboBox.WidestText
                                }
                                Button {
                                    text: "Добавить карту"
                                    onClicked: {
                                        if (accModel.count === 0) { addCardStatus.text = "Сначала создайте счет"; return }
                                        const accNum = accModel.get(accCombo.currentIndex).value
                                        if (!cardHolder.text || cardHolder.text.length < 2) { addCardStatus.text = "Имя владельца слишком короткое"; return }
                                        if (!/^\d{2}\/\d{2}$/.test(cardExpiry.text)) { addCardStatus.text = "Срок в формате ММ/ГГ"; return }
                                        bank.addCard(cardHolder.text, cardExpiry.text, accNum)
                                        // мгновенно обновим списки
                                        cardsList.model = bank.listCards()
                                        accRefresh()
                                    }
                                }
                            }
                            Label { id: addCardStatus; text: ""; color: "#666" }
                            ListView {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                id: cardsList
                                model: bank.listCards()
                                spacing: 8
                                delegate: Frame {
                                    width: ListView.view.width
                                    property bool expired: checkCardExpiry(modelData.expiry)
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 2
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Label { 
                                                text: "Карта: " + modelData.cardNumber; 
                                                font.bold: true
                                                Layout.fillWidth: true
                                            }
                                            Label {
                                                text: expired ? "ПРОСРОЧЕНА" : "Действительна"
                                                color: expired ? "#d32f2f" : "#4caf50"
                                                font.bold: true
                                                font.pixelSize: 11
                                            }
                                        }
                                        Label { text: "Владелец: " + modelData.holderName }
                                        Label { 
                                            text: "Срок: " + modelData.expiry
                                            color: expired ? "#d32f2f" : "#666"
                                        }
                                        Label { text: "Счет: " + modelData.linkedAccount }
                                    }
                                }
                            }
                        }
                    }

                    // Transfers + History (panel on top, history below)
                    Item {
                        ColumnLayout {
                            visible: !bank.admin
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            Frame {
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8
                                    Label { text: "Новый перевод"; font.bold: true }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Счет отправителя:"; Layout.preferredWidth: 140 }
                                        ComboBox { 
                                            id: fromAcc
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 300
                                            textRole: "text"
                                            model: accModel
                                            popup.width: Math.max(width, implicitContentWidth + 40)
                                            implicitContentWidthPolicy: ComboBox.WidestText
                                        }
                                    }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Имя получателя:"; Layout.preferredWidth: 140 }
                                        TextField { id: recipient; Layout.fillWidth: true; placeholderText: "Имя получателя"; onTextChanged: { refreshRecipientCards(); refreshRecipientAccounts(); } }
                                        Button { text: "Найти"; onClicked: { refreshRecipientCards(); refreshRecipientAccounts(); } }
                                    }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Карта получателя:"; Layout.preferredWidth: 140 }
                                        ComboBox { 
                                            id: recipientCards
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 300
                                            textRole: "text"
                                            model: recipientCardsModel
                                            popup.width: Math.max(width, implicitContentWidth + 40)
                                            implicitContentWidthPolicy: ComboBox.WidestText
                                        }
                                    }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Счет получателя:"; Layout.preferredWidth: 140 }
                                        ComboBox { 
                                            id: recipientAccounts
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 300
                                            textRole: "text"
                                            model: recipientAccountsModel
                                            popup.width: Math.max(width, implicitContentWidth + 40)
                                            implicitContentWidthPolicy: ComboBox.WidestText
                                        }
                                    }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Сумма:"; Layout.preferredWidth: 140 }
                                        SpinBox { id: amount; Layout.fillWidth: true; from: 1; to: 100000000; value: 1000; editable: true }
                                    }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Категория:"; Layout.preferredWidth: 140 }
                                        ComboBox {
                                            id: categoryCombo
                                            Layout.fillWidth: true
                                            model: ListModel {
                                                ListElement { text: "Остальное"; value: "other" }
                                                ListElement { text: "Медицина и здравоохранение"; value: "medicine" }
                                                ListElement { text: "Спорт"; value: "sport" }
                                                ListElement { text: "Продукты"; value: "food" }
                                                ListElement { text: "Развлечения"; value: "entertainment" }
                                            }
                                            textRole: "text"
                                        }
                                    }
                                    RowLayout {
                                        spacing: 6
                                        Label { text: "Примечание:"; Layout.preferredWidth: 140 }
                                        TextField { id: note; Layout.fillWidth: true; placeholderText: "Примечание" }
                                    }
                                    Button {
                                        text: "Отправить"
                                        Layout.fillWidth: true
                                        onClicked: {
                                            var target = ""
                                            if (recipientCardsModel.count > 0 && recipientCards.currentIndex >= 0) {
                                                target = recipientCardsModel.get(recipientCards.currentIndex).value
                                            } else if (recipientAccountsModel.count > 0 && recipientAccounts.currentIndex >= 0) {
                                                target = recipientAccountsModel.get(recipientAccounts.currentIndex).value
                                            }
                                            if (!target) { transferStatus.text = "Укажите получателя и выберите карту/счет"; return }
                                            if (accModel.count === 0 || fromAcc.currentIndex < 0) { transferStatus.text = "Создайте и выберите свой счет"; return }
                                            const myAcc = accModel.get(fromAcc.currentIndex).value
                                            const cat = categoryCombo.currentIndex >= 0 ? categoryCombo.model.get(categoryCombo.currentIndex).value : "other"
                                            bank.transfer(myAcc, target, amount.value*100, note.text, cat)
                                        }
                                    }
                                }
                            }
                            Label { id: transferStatus; text: ""; color: "#666" }
                            // History header
                            RowLayout {
                                spacing: 12
                                Layout.fillWidth: true
                                Label { text: "Ваш счет"; Layout.preferredWidth: 220; font.bold: true }
                                Label { text: "Карта/счет получателя"; Layout.preferredWidth: 280; font.bold: true }
                                Label { text: "Сумма"; Layout.preferredWidth: 120; font.bold: true }
                                Label { text: "Дата/время"; Layout.preferredWidth: 360; font.bold: true }
                                Label { text: "Статус"; Layout.preferredWidth: 120; font.bold: true }
                                Label { text: "Примечание"; Layout.fillWidth: true; font.bold: true }
                                Label { text: ""; Layout.preferredWidth: 80 }
                            }
                            // History below
                            ListView {
                                id: historyList
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                model: bank.listHistory()
                                delegate: RowLayout {
                                    width: ListView.view.width
                                    spacing: 12
                                    // columns aligned with header widths
                                    Label { text: modelData.fromAccount; Layout.preferredWidth: 220 }
                                    Label { text: modelData.toCard; Layout.preferredWidth: 280 }
                                    Label { text: (modelData.cents/100).toFixed(2); Layout.preferredWidth: 120 }
                                    Text { text: new Date(modelData.timestamp*1000).toLocaleString(); Layout.preferredWidth: 360; elide: Text.ElideRight; wrapMode: Text.WordWrap; maximumLineCount: 2 }
                                    Label {
                                        text: modelData.status === "cancelled" ? "Отменен" : "Выполнен"
                                        Layout.preferredWidth: 120
                                        color: modelData.status === "cancelled" ? "tomato" : "#18a558"
                                    }
                                    Text {
                                        text: modelData.status === "cancelled" && modelData.cancelReason.length ? modelData.note + " (" + modelData.cancelReason + ")" : modelData.note
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                    }
                                    Button {
                                        text: "Чек"
                                        Layout.preferredWidth: 80
                                        onClicked: showReceipt(modelData.id)
                                    }
                                }
                            }
                        }
                    }


                    // Favorites (with selectors)
                    Item {
                        ColumnLayout {
                            visible: !bank.admin
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            RowLayout {
                                spacing: 6
                                TextField { id: favName; placeholderText: "Название" }
                                TextField { id: favRecipient; placeholderText: "Имя получателя"; onTextChanged: { refreshRecipientCards(); refreshRecipientAccounts(); } }
                                ComboBox { 
                                    id: favCard
                                    Layout.preferredWidth: 400
                                    Layout.minimumWidth: 300
                                    Layout.fillWidth: true
                                    textRole: "text"
                                    model: recipientCardsModel
                                    popup.width: Math.max(400, implicitContentWidth + 40)
                                    implicitContentWidthPolicy: ComboBox.WidestText
                                }
                                TextField { id: favNote; placeholderText: "Примечание" }
                                Button { text: "Добавить"; onClicked: {
                                        if (recipientCardsModel.count === 0) { addCardStatus.text = "Укажите получателя и выберите карту"; return }
                                        const toCardNum = recipientCardsModel.get(favCard.currentIndex).value
                                        bank.addFavorite(favName.text, toCardNum, favNote.text)
                                        favRefresh()
                                    } }
                            }
                            ListView {
                                id: favoritesList
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                model: bank.listFavorites()
                                delegate: Frame {
                                    width: ListView.view.width
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 6
                                        RowLayout {
                                            spacing: 12
                                            Label { text: modelData.name; font.bold: true }
                                            Label { text: modelData.toCard }
                                            Label { text: modelData.note }
                                        }
                                        RowLayout {
                                            spacing: 6
                                            ComboBox { 
                                                id: favFromAcc
                                                Layout.preferredWidth: 400
                                                Layout.minimumWidth: 300
                                                Layout.fillWidth: true
                                                textRole: "text"
                                                model: accModel
                                                popup.width: Math.max(400, implicitContentWidth + 40)
                                                implicitContentWidthPolicy: ComboBox.WidestText
                                            }
                                            SpinBox { id: favAmount; from: 1; to: 100000000; value: 1000; editable: true }
                                            ComboBox {
                                                id: favCategoryCombo
                                                Layout.preferredWidth: 200
                                                model: ListModel {
                                                    ListElement { text: "Остальное"; value: "other" }
                                                    ListElement { text: "Медицина и здравоохранение"; value: "medicine" }
                                                    ListElement { text: "Спорт"; value: "sport" }
                                                    ListElement { text: "Продукты"; value: "food" }
                                                    ListElement { text: "Развлечения"; value: "entertainment" }
                                                }
                                                textRole: "text"
                                            }
                                            Button { text: "Оплатить"; onClicked: {
                                                    if (accModel.count === 0 || favFromAcc.currentIndex < 0) { addCardStatus.text = "Выберите свой счет"; return }
                                                    const myAcc = accModel.get(favFromAcc.currentIndex).value
                                                    const cat = favCategoryCombo.currentIndex >= 0 ? favCategoryCombo.model.get(favCategoryCombo.currentIndex).value : "other"
                                                    bank.payFavorite(modelData.name, myAcc, favAmount.value*100, cat)
                                                } }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Statement (Выписка с круговой диаграммой)
                    Item {
                        property alias expenseModel: expenseListModel
                        Component.onCompleted: {
                            if (typeof updateExpenseChart !== 'undefined') {
                                updateExpenseChart()
                            }
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 20
                            visible: !bank.admin
                            
                            // Круговая диаграмма (слева)
                            Frame {
                                Layout.preferredWidth: 380
                                Layout.fillHeight: true
                                Canvas {
                                    id: expenseChart
                                    anchors.centerIn: parent
                                    width: Math.min(parent.width - 20, parent.height - 20, 360)
                                    height: width
                                    
                                    Connections {
                                        target: expenseListModel
                                        function onCountChanged() {
                                            expenseChart.requestPaint()
                                        }
                                    }
                                    
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        
                                        var stats = bank.getExpenseStats()
                                        if (!stats || !stats.total || stats.total === 0) {
                                            ctx.fillStyle = "#999"
                                            ctx.font = "20px sans-serif"
                                            ctx.textAlign = "center"
                                            ctx.fillText("Нет данных", width/2, height/2)
                                            return
                                        }
                                        
                                        var centerX = width / 2
                                        var centerY = height / 2
                                        var radius = Math.max(10, Math.min(width, height) / 2 - 20)
                                        
                                        var colors = {
                                            "medicine": "#4CAF50",
                                            "sport": "#2196F3",
                                            "food": "#FF9800",
                                            "entertainment": "#9C27B0",
                                            "other": "#607D8B"
                                        }
                                        
                                        var categories = ["medicine", "sport", "food", "entertainment", "other"]
                                        var startAngle = -Math.PI / 2
                                        
                                        for (var i = 0; i < categories.length; i++) {
                                            var cat = categories[i]
                                            var catData = stats[cat]
                                            if (!catData || !catData.amount || catData.amount === 0) continue
                                            
                                            var percent = (catData.percent || 0) / 100
                                            if (percent <= 0) continue
                                            
                                            var endAngle = startAngle + percent * 2 * Math.PI
                                            
                                            ctx.save()
                                            ctx.beginPath()
                                            ctx.moveTo(centerX, centerY)
                                            ctx.arc(centerX, centerY, radius, startAngle, endAngle, false)
                                            ctx.closePath()
                                            
                                            ctx.fillStyle = colors[cat] || "#607D8B"
                                            ctx.fill()
                                            
                                            ctx.strokeStyle = "#ffffff"
                                            ctx.lineWidth = 2
                                            ctx.stroke()
                                            ctx.restore()
                                            
                                            startAngle = endAngle
                                        }
                                    }
                                }
                            }
                            
                            // Детализация расходов (справа)
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                ColumnLayout {
                                    width: parent.width
                                    spacing: 12
                                    Repeater {
                                        id: expenseRepeater
                                        model: expenseListModel
                                        delegate: Frame {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 70
                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 12
                                                spacing: 16
                                                Rectangle {
                                                    Layout.preferredWidth: 24
                                                    Layout.preferredHeight: 24
                                                    Layout.alignment: Qt.AlignVCenter
                                                    radius: 4
                                                    color: model.color || "#000"
                                                }
                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    Layout.alignment: Qt.AlignVCenter
                                                    spacing: 6
                                                    Label {
                                                        text: model.name || "Неизвестно"
                                                        font.bold: true
                                                        font.pixelSize: 15
                                                        Layout.fillWidth: true
                                                    }
                                                    Label {
                                                        text: "Сумма: " + ((Number(model.amount) || 0)/100).toFixed(2) + " руб."
                                                        font.pixelSize: 13
                                                        color: "#666"
                                                        Layout.fillWidth: true
                                                    }
                                                }
                                                Item {
                                                    Layout.preferredWidth: 20
                                                }
                                                Label {
                                                    text: (Number(model.percent) || 0).toFixed(1) + "%"
                                                    font.bold: true
                                                    font.pixelSize: 18
                                                    color: model.color || "#000"
                                                    Layout.alignment: Qt.AlignVCenter
                                                    Layout.preferredWidth: 60
                                                    horizontalAlignment: Text.AlignRight
                                                }
                                            }
                                        }
                                    }
                                    Label {
                                        visible: expenseListModel.count === 0
                                        text: "Нет данных о расходах"
                                        color: "#999"
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.topMargin: 20
                                    }
                                }
                            }
                        }
                    }

                    // Notifications
                    Item {
                        ColumnLayout {
                            visible: !bank.admin
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            RowLayout {
                                spacing: 8
                                Button { text: "Обновить"; onClicked: notificationsView.model = bank.listNotifications() }
                                Button { text: "Очистить"; onClicked: bank.clearNotifications() }
                            }
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                ListView {
                                    id: notificationsView
                                    width: parent.width
                                    model: bank.listNotifications()
                                    spacing: 8
                                    delegate: Frame {
                                        width: ListView.view.width
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 12
                                            Rectangle {
                                                Layout.preferredWidth: 8
                                                Layout.preferredHeight: 8
                                                radius: 4
                                                color: "#FF9800"
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                wrapMode: Text.WordWrap
                                                text: modelData.message
                                                font.pixelSize: 13
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Admin Panel - Payments
                    Item {
                        ColumnLayout {
                            visible: bank.admin
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            Label { 
                                text: "Все платежи"; 
                                font.bold: true; 
                                font.pixelSize: 18; 
                                Layout.fillWidth: true 
                            }
                            RowLayout {
                                spacing: 8
                                TextField { 
                                    id: adminSearchField
                                    placeholderText: "Поиск по пользователю, ID, счету, карте..."
                                    Layout.fillWidth: true
                                    onTextChanged: {
                                        if (typeof adminTransfersList !== 'undefined') {
                                            adminTransfersList.model = bank.listAllTransfers(text)
                                        }
                                    }
                                }
                                Button { 
                                    text: "Обновить"; 
                                    onClicked: {
                                        if (typeof adminTransfersList !== 'undefined') {
                                            adminTransfersList.model = bank.listAllTransfers(adminSearchField.text)
                                        }
                                    } 
                                }
                            }
                            // Header
                            RowLayout {
                                spacing: 12
                                Layout.fillWidth: true
                                Label { text: "Пользователь"; Layout.preferredWidth: 120; font.bold: true }
                                Label { text: "ID платежа"; Layout.preferredWidth: 150; font.bold: true }
                                Label { text: "Отправитель"; Layout.preferredWidth: 150; font.bold: true }
                                Label { text: "Получатель"; Layout.preferredWidth: 150; font.bold: true }
                                Label { text: "Сумма"; Layout.preferredWidth: 100; font.bold: true }
                                Label { text: "Дата/время"; Layout.preferredWidth: 180; font.bold: true }
                                Label { text: "Статус"; Layout.preferredWidth: 100; font.bold: true }
                                Label { text: "Примечание"; Layout.fillWidth: true; font.bold: true }
                                Label { text: ""; Layout.preferredWidth: 100 }
                            }
                            // List
                            ListView {
                                id: adminTransfersList
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                model: bank.listAllTransfers("")
                                spacing: 4
                                delegate: RowLayout {
                                    width: ListView.view.width
                                    spacing: 12
                                    Label { 
                                        text: modelData.user || ""; 
                                        Layout.preferredWidth: 120 
                                    }
                                    Label { 
                                        text: modelData.id || ""; 
                                        Layout.preferredWidth: 150;
                                        font.family: "monospace"
                                        font.pixelSize: 11
                                    }
                                    Label { 
                                        text: modelData.fromAccount || ""; 
                                        Layout.preferredWidth: 150;
                                        font.family: "monospace"
                                        font.pixelSize: 11
                                    }
                                    Label { 
                                        text: modelData.toCard || ""; 
                                        Layout.preferredWidth: 150;
                                        font.family: "monospace"
                                        font.pixelSize: 11
                                    }
                                    Label { 
                                        text: ((modelData.cents || 0)/100).toFixed(2) + " руб."; 
                                        Layout.preferredWidth: 100 
                                    }
                                    Text { 
                                        text: modelData.timestamp ? new Date(modelData.timestamp*1000).toLocaleString() : ""; 
                                        Layout.preferredWidth: 180; 
                                        elide: Text.ElideRight;
                                        wrapMode: Text.WordWrap;
                                        maximumLineCount: 2
                                    }
                                    Label {
                                        text: modelData.status === "cancelled" ? "Отменен" : "Выполнен"
                                        Layout.preferredWidth: 100
                                        color: modelData.status === "cancelled" ? "tomato" : "#18a558"
                                        font.bold: true
                                    }
                                    Text {
                                        text: (modelData.status === "cancelled" && modelData.cancelReason ? 
                                               (modelData.note || "") + " (Отмена: " + modelData.cancelReason + ")" : 
                                               (modelData.note || ""))
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                    }
                                    Button {
                                        text: modelData.status === "cancelled" ? "Отменен" : "Отменить"
                                        Layout.preferredWidth: 100
                                        enabled: modelData.status !== "cancelled"
                                        onClicked: {
                                            adminCancelTxId = modelData.id
                                            adminCancelReason.text = ""
                                            adminCancelDialog.open()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Admin Panel - Users
                    Item {
                        ColumnLayout {
                            visible: bank.admin
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            Label { 
                                text: "Пользователи"; 
                                font.bold: true; 
                                font.pixelSize: 18; 
                                Layout.fillWidth: true 
                            }
                            RowLayout {
                                spacing: 8
                                Label { text: "Сортировка:" }
                                ComboBox {
                                    id: adminUsersSort
                                    Layout.preferredWidth: 200
                                    model: ListModel {
                                        ListElement { text: "По имени" }
                                        ListElement { text: "По количеству счетов" }
                                        ListElement { text: "По количеству карт" }
                                        ListElement { text: "По количеству транзакций" }
                                    }
                                    onCurrentTextChanged: {
                                        let sortValue = ""
                                        if (currentText === "По количеству счетов") sortValue = "accounts"
                                        else if (currentText === "По количеству карт") sortValue = "cards"
                                        else if (currentText === "По количеству транзакций") sortValue = "transactions"
                                        if (typeof adminUsersList !== 'undefined') {
                                            adminUsersList.model = bank.getAllUsersInfo(sortValue)
                                        }
                                    }
                                }
                                Item { Layout.fillWidth: true }
                                Button { 
                                    text: "Обновить"; 
                                    onClicked: {
                                        if (typeof adminUsersList !== 'undefined') {
                                            let sortValue = ""
                                            if (adminUsersSort.currentText === "По количеству счетов") sortValue = "accounts"
                                            else if (adminUsersSort.currentText === "По количеству карт") sortValue = "cards"
                                            else if (adminUsersSort.currentText === "По количеству транзакций") sortValue = "transactions"
                                            adminUsersList.model = bank.getAllUsersInfo(sortValue)
                                        }
                                    } 
                                }
                            }
                            // Header
                            RowLayout {
                                spacing: 16
                                Layout.fillWidth: true
                                Label { text: "Пользователь"; Layout.preferredWidth: 180; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignLeft }
                                Label { text: "Счетов"; Layout.preferredWidth: 100; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter }
                                Label { text: "Карт"; Layout.preferredWidth: 100; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter }
                                Label { text: "Транзакций"; Layout.preferredWidth: 120; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter }
                                Label { text: "Общий баланс"; Layout.preferredWidth: 150; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignRight }
                                Label { text: "Избранных"; Layout.preferredWidth: 120; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter }
                                Label { text: "Уведомлений"; Layout.preferredWidth: 120; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter }
                                Label { text: ""; Layout.preferredWidth: 120 }
                            }
                            // List
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                ListView {
                                    id: adminUsersList
                                    width: parent.width
                                    model: bank.getAllUsersInfo("")
                                    spacing: 8
                                    delegate: Item {
                                        id: userItem
                                        width: ListView.view.width
                                        implicitHeight: mainColumn.implicitHeight + 24
                                        property bool expanded: false
                                        Frame {
                                            anchors.fill: parent
                                            clip: true
                                            padding: 12
                                            ColumnLayout {
                                                id: mainColumn
                                                anchors.fill: parent
                                                spacing: 10
                                                RowLayout {
                                                    id: mainRow
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 36
                                                    spacing: 16
                                                    Label { 
                                                        text: modelData.username || ""; 
                                                        Layout.preferredWidth: 180;
                                                        Layout.maximumWidth: 180
                                                        elide: Text.ElideRight
                                                        horizontalAlignment: Text.AlignLeft
                                                        font.bold: true
                                                        font.pixelSize: 13
                                                    }
                                                    Label { 
                                                        text: (modelData.accountsCount || 0).toString(); 
                                                        Layout.preferredWidth: 100
                                                        Layout.maximumWidth: 100
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.pixelSize: 13
                                                    }
                                                    Label { 
                                                        text: (modelData.cardsCount || 0).toString(); 
                                                        Layout.preferredWidth: 100
                                                        Layout.maximumWidth: 100
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.pixelSize: 13
                                                    }
                                                    Label { 
                                                        text: (modelData.transactionsCount || 0).toString(); 
                                                        Layout.preferredWidth: 120
                                                        Layout.maximumWidth: 120
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.pixelSize: 13
                                                    }
                                                    Label { 
                                                        text: ((modelData.totalBalance || 0)/100).toFixed(2) + " руб."; 
                                                        Layout.preferredWidth: 150
                                                        Layout.maximumWidth: 150
                                                        elide: Text.ElideRight
                                                        horizontalAlignment: Text.AlignRight
                                                        font.pixelSize: 13
                                                    }
                                                    Label { 
                                                        text: (modelData.favoritesCount || 0).toString(); 
                                                        Layout.preferredWidth: 120
                                                        Layout.maximumWidth: 120
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.pixelSize: 13
                                                    }
                                                    Label { 
                                                        text: (modelData.notificationsCount || 0).toString(); 
                                                        Layout.preferredWidth: 120
                                                        Layout.maximumWidth: 120
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.pixelSize: 13
                                                    }
                                                    Button {
                                                        text: userItem.expanded ? "Свернуть" : "Детали"
                                                        Layout.preferredWidth: 120
                                                        Layout.maximumWidth: 120
                                                        font.pixelSize: 12
                                                        onClicked: {
                                                            userItem.expanded = !userItem.expanded
                                                        }
                                                    }
                                                }
                                                // Детальная информация (счета и карты)
                                                ColumnLayout {
                                                    id: detailsColumn
                                                    visible: userItem.expanded
                                                    Layout.fillWidth: true
                                                    Layout.topMargin: 4
                                                    spacing: 8
                                                    // Счета
                                                    Label {
                                                        text: "Счета:"
                                                        font.bold: true
                                                        font.pixelSize: 12
                                                        visible: (modelData.accountsCount || 0) > 0
                                                    }
                                                    Repeater {
                                                        model: modelData.accounts || []
                                                        delegate: RowLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredHeight: 20
                                                            Label {
                                                                text: "  • " + (modelData.accountNumber || "") + " (" + (modelData.currency || "") + "): " + ((modelData.balanceCents || 0)/100).toFixed(2) + " руб."
                                                                Layout.fillWidth: true
                                                                elide: Text.ElideRight
                                                                font.pixelSize: 11
                                                                color: "#666"
                                                            }
                                                        }
                                                    }
                                                    // Карты
                                                    Label {
                                                        text: "Карты:"
                                                        font.bold: true
                                                        font.pixelSize: 12
                                                        visible: (modelData.cardsCount || 0) > 0
                                                    }
                                                    Repeater {
                                                        model: modelData.cards || []
                                                        delegate: RowLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredHeight: 20
                                                            Label {
                                                                text: "  • " + (modelData.cardNumber || "") + " (" + (modelData.holderName || "") + ") - Срок: " + (modelData.expiry || "")
                                                                Layout.fillWidth: true
                                                                elide: Text.ElideRight
                                                                font.pixelSize: 11
                                                                color: "#666"
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                }
            }
            Connections {
                target: bank
                function onAuthenticatedChanged() {
                    authStatus.text = bank.authenticated ? "Вход выполнен: " + bank.username : ""
                    if (!bank.authenticated) {
                        stack.replace(loginPage)
                        if (typeof notificationsView !== 'undefined') notificationsView.model = []
                        if (typeof historyList !== 'undefined') historyList.model = []
                        if (typeof favoritesList !== 'undefined') favoritesList.model = []
                    } else {
                        accRefresh()
                        accountsList.model = bank.listAccounts()
                        cardsList.model = bank.listCards()
                        if (typeof historyList !== 'undefined') historyList.model = bank.listHistory()
                        favRefresh()
                        if (typeof notificationsView !== 'undefined') notificationsView.model = bank.listNotifications()
                        if (typeof updateExpenseChart !== 'undefined') updateExpenseChart()
                        if (typeof adminTransfersList !== 'undefined' && bank.admin) {
                            adminTransfersList.model = bank.listAllTransfers("")
                        }
                        if (typeof adminUsersList !== 'undefined' && bank.admin) {
                            adminUsersList.model = bank.getAllUsersInfo("")
                        }
                        contentView.currentIndex = 0
                    }
                }
                function onInfoMessage(message) {
                    accStatus.text = message; addCardStatus.text = message; transferStatus.text = message;
                    accRefresh();
                    accountsList.model = bank.listAccounts();
                    cardsList.model = bank.listCards();
                    if (typeof historyList !== 'undefined') historyList.model = bank.listHistory();
                    favRefresh();
                    if (typeof notificationsView !== 'undefined') notificationsView.model = bank.listNotifications()
                    if (typeof updateExpenseChart !== 'undefined') updateExpenseChart()
                    if (typeof adminTransfersList !== 'undefined' && bank.admin) {
                        adminTransfersList.model = bank.listAllTransfers(adminSearchField ? adminSearchField.text : "")
                    }
                    authStatus.text = bank.authenticated ? "Вход выполнен: " + bank.username : ""
                }
                function onErrorOccured(message) {
                    accStatus.text = message; addCardStatus.text = message; transferStatus.text = message
                    authStatus.text = bank.authenticated ? "Вход выполнен: " + bank.username : ""
                }
            }

            Dialog {
                id: depositDialog
                modal: true
                title: "Пополнение счета"
                standardButtons: Dialog.Ok | Dialog.Cancel
                onAccepted: {
                    if (depositAccount.currentIndex < 0 || accModel.count === 0) { accStatus.text = "Выберите счет"; return }
                    const accNum = accModel.get(depositAccount.currentIndex).value
                    bank.depositToAccount(accNum, depositAmount.value * 100, depositExternal.text)
                }
                ColumnLayout {
                    anchors.margins: 12
                    spacing: 8
                    ComboBox { 
                        id: depositAccount
                        Layout.preferredWidth: 450
                        Layout.minimumWidth: 350
                        Layout.fillWidth: true
                        textRole: "text"
                        model: accModel
                        popup.width: Math.max(450, implicitContentWidth + 40)
                        implicitContentWidthPolicy: ComboBox.WidestText
                    }
                    TextField { id: depositExternal; placeholderText: "Номер внешнего счета" }
                    SpinBox { id: depositAmount; from: 1; to: 100000000; value: 1000; editable: true }
                }
            }

            Dialog {
                id: receiptDialog
                modal: true
                title: "Чек"
                standardButtons: Dialog.Ok
                ColumnLayout {
                    anchors.margins: 12
                    spacing: 6
                    Label { text: "Пользователь: " + (receiptData.user || bank.username) }
                    Label { text: "Отправитель: " + (receiptData.fromAccount || "") }
                    Label { text: "Получатель: " + (receiptData.toCard || "") }
                    Label { text: "Сумма: " + ((receiptData.cents || 0)/100).toFixed(2) }
                    Label { text: "Статус: " + (receiptData.status || "") }
                    Label { text: "Примечание: " + (receiptData.note || "") }
                    Label { 
                        visible: receiptData.cancelReason !== undefined && receiptData.cancelReason !== null && String(receiptData.cancelReason).length > 0
                        text: "Причина отмены: " + (receiptData.cancelReason || "") 
                    }
                    Label { text: receiptData.timestamp ? ("Дата: " + new Date(receiptData.timestamp*1000).toLocaleString()) : "" }
                    Button {
                        text: "Скачать чек"
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: downloadReceipt()
                    }
                }
            }

            FileDialog {
                id: receiptFileDialog
                title: "Сохранить чек"
                fileMode: FileDialog.SaveFile
                nameFilters: ["Текстовые файлы (*.txt)", "Все файлы (*)"]
                defaultSuffix: "txt"
                onAccepted: {
                    if (receiptData && receiptData.id) {
                        let filePath = selectedFile
                        // В Qt 6 FileDialog selectedFile это QUrl, используем toLocalFile()
                        if (typeof filePath.toLocalFile === 'function') {
                            filePath = filePath.toLocalFile()
                        } else {
                            // Fallback для старых версий
                            filePath = filePath.toString().replace(/^file:\/\//, "")
                        }
                        bank.saveReceiptToFile(receiptData.id, filePath)
                    }
                }
            }

            Dialog {
                id: cancelDialog
                modal: true
                title: "Отмена платежа"
                standardButtons: Dialog.Ok | Dialog.Cancel
                onAccepted: submitCancel()
                ColumnLayout {
                    anchors.margins: 12
                    spacing: 8
                    TextField { id: cancelReason; placeholderText: "Причина отмены" }
                }
            }

            Dialog {
                id: adminCancelDialog
                modal: true
                title: "Отмена платежа (Администратор)"
                standardButtons: Dialog.Ok | Dialog.Cancel
                onAccepted: {
                    if (!adminCancelTxId) return
                    bank.cancelTransfer(adminCancelTxId, adminCancelReason.text)
                    adminCancelTxId = ""
                    if (typeof adminTransfersList !== 'undefined') {
                        adminTransfersList.model = bank.listAllTransfers(adminSearchField.text)
                    }
                }
                ColumnLayout {
                    anchors.margins: 12
                    spacing: 8
                    Label { 
                        text: "ID платежа: " + adminCancelTxId
                        font.bold: true
                    }
                    TextField { 
                        id: adminCancelReason
                        placeholderText: "Причина отмены платежа"
                        Layout.fillWidth: true
                    }
                }
            }
            Component.onCompleted: {
                accRefresh()
                if (typeof notificationsView !== 'undefined') notificationsView.model = bank.listNotifications()
                authStatus.text = bank.authenticated ? "Вход выполнен: " + bank.username : ""
            }
        }
    }
}
