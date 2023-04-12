try {
    vscode = acquireVsCodeApi();
} catch (error) {}
curr_row = undefined;

function getSelectionText()//(rowNum) {}
{
    // var activeEl = document.activeElement;
    // var activeElTagName = activeEl ? activeEl.tagName.toLowerCase() : null;
    // if (activeElTagName==null) {
    //     return "";
    // }

    startSelectionTextNode = window.getSelection().anchorNode;
    endSelectionTextNode = window.getSelection().focusNode;
    //or window.getSelection().anchorNode.parentNode is not exampleCell (its under font or span)
    //was here: make sure selection is in exampleCell & one exampleCell, from closest('tr') (parent.parent actually) getElement literal or general
    //maybe split getText to raw getText and wrapper getExampleText
    if (startSelectionTextNode==null
        || startSelectionTextNode!==endSelectionTextNode
        || !startSelectionTextNode.parentNode.classList.contains("exampleCell")){//.tagName.toLowerCase()!="td") {
        //now i check if it is continuous, not wven fonts or spans inside, so im not coloring a colored letter
        // startSelectionTextNode.parentNode.closest("tr")!==startSelectionTextNode.parentNode.closest("tr") will check if same example row
        return "";
    }
    curr_row = window.getSelection().focusNode.parentNode.closest("tr");

    var text = "";
    // if (window.getSelection().anchorNode.parentNode=="td .tg-white_background .exampleCell"
    //     && window.getSelection().anchorNode.parentNode.parentNode=="tr#row"+rowNum) {
        if (window.getSelection) {
            text = window.getSelection().toString();
        } else if (document.selection && document.selection.type != "Control") {
            text = document.selection.createRange().text;
        }
    // }
    return text;
}

function addChars(cellClass,chars)
{
    if (curr_row===undefined || chars==="") {
        return false;
    }
    curr_cell = curr_row.getElementsByClassName(cellClass)[0];
    curr_row=undefined;
    if (curr_cell===undefined) {
        return false;
    }
    curr_text = curr_cell.innerText;
    for (let c = 0; c < chars.length; c++) {
        if (curr_text.indexOf(chars[c])==-1) {
            curr_text+=chars[c];
        }
    }
    curr_cell.innerText = curr_text;
    return true;
}

function colorize(aCommandName, aShowDefaultUI, aValueArgument)
{
    // Get Selection
    sel = window.getSelection();
    range = undefined;
    if (sel.rangeCount && sel.getRangeAt) {
        range = sel.getRangeAt(0);
    }
    // Set design mode to on
    document.designMode = "on";
    if (range) {
        sel.removeAllRanges();
        sel.addRange(range);
    }
    // Colorize text
    document.execCommand(aCommandName, aShowDefaultUI, aValueArgument);
    // Set design mode to off
    document.designMode = "off";
}

function markLiteral()
{
    if (addChars("literalsCell",getSelectionText())) {
        if (getSelectionText()===" ")
            colorize("ForeColor", false, "#ffbf00");
    }
}

gen_text = undefined;

function markGeneral_tx()
{
    gen_text = getSelectionText();
    if (gen_text != "") {
		vscode.postMessage({command: "getGeneral"});
    } else {
		gen_text = undefined;
	}
}

function markGeneral_rx(gen_type)
{
    if (addChars("generalsCell_"+gen_type,gen_text)) {
		colorize("hilitecolor", false, "cornflowerblue");
	}
	gen_text = undefined;
}

numOfRows = 0;

const row = '\
<td class="tg-white_background removeCell"><button type="button" onclick="removeRow(this)">Remove Row</button></td>\
    <td class="tg-white_background keep-space exampleCell"><input type="text" name="example" placeholder="Enter your example" size="28" id="example" required></td> \
    <td class="tg-white_background accRejCell">                                                                                                          \
        <select name="acc_or_rej" id="acc_or_rej">                                                                                           \
            <option value="acc" selected="selected">Accept</option>                                                                          \
            <option value="rej">Reject</option>                                                                                              \
        </select>                                                                                                                            \
    </td>                                                                                                                                    \
    <td class="tg-white_background keep-space literalsCell"></td>                                                                                                   \
    <td class="tg-white_background keep-space generalsCell_num"></td>                                                                                               \
    <td class="tg-white_background keep-space generalsCell_num1-9"></td>                                                                                            \
    <td class="tg-white_background keep-space generalsCell_let"></td>                                                                                               \
    <td class="tg-white_background keep-space generalsCell_low"></td>                                                                                               \
    <td class="tg-white_background keep-space generalsCell_cap"></td>                                                                                               \
    <td class="tg-white_background keep-space generalsCell_any"></td>                                                                                               \
'
function addRow()
{
    new_row = document.createElement("tr");
    new_row.setAttribute("id","row"+ numOfRows++);
    new_row.innerHTML = row;
    document.getElementById("examples").appendChild(new_row);
}

function removeRow(buttonElem)
{
    if (buttonElem.closest('tr').children[1].textContent!=="") {
        let current_row = buttonElem.closest('tr');
        current_row.remove();
        --numOfRows;
    }
}

function storeExample()
{
    newExampleRow = document.getElementById("examples").lastChild;
    exampleCell = newExampleRow.getElementsByTagName("td")[1];
    example = exampleCell.lastChild.value;
    if (example==="") {
        return false;
    }
    exampleCell.innerText = example;
    return true;
}

function submitExample()
{
    if (storeExample()) {
        addRow();
    }
}

function buildInput()
{
    input = {};
    //will be mixed so I dont really need to organize info by its example
    //{
    //   rowNum: {example: <exampleCell>,result:<accRejCell>,literals:<literalCell>,generals:["column" for cell if it has letters]}
    //}

    // rows = document.getElementById("examples").childNodes;
    // current_row = rows[i].children;

    // exampleCells = document.getElementById("examples").getElementsByClassName("exampleCell");
    // input[i]["example"] <- exampleCells[i]

    rows = document.getElementById("examples").childNodes;
    for (let i=0; i<numOfRows-1; ++i) {
        current_row = rows[i].children;
        input[i]={};
        input[i]["example"] = current_row[1].innerText;
        input[i]["result"] = current_row[2].children[0].value;
        input[i]["literals"] = current_row[3].innerText;
        input[i]["generals"] = [];
        cols = ["num","num1-9","let","low","cap","any"];
        for (let j=0; j<cols.length; ++j) {
            if (current_row[4+j].innerText!=="") {
                input[i]["generals"].push(cols[j]);
            }
        }
    }
    if (Object.keys(input).length > 0) {
        input["include"] = [];
        if (document.getElementById("includes") !== null) {
            for (let i = 0; i < document.getElementById("includes").childNodes.length; i++) {
                if (document.getElementById("includes").childNodes[i].classList.contains("includeCell")) {
                    let includeText = document.getElementById("includes").childNodes[i].lastChild.textContent.trim();
                    if (includeText !== "") {
                        input["include"].push(includeText);
                    }
                }
            }
        }
        input["exclude"]  = [];
        if (document.getElementById("excludes") !== null) {
            for (let i = 0; i < document.getElementById("excludes").childNodes.length; i++) {
                if (document.getElementById("excludes").childNodes[i].classList.contains("excludeCell")) {
                    let includeText = document.getElementById("excludes").childNodes[i].lastChild.textContent.trim();
                    if (includeText !== "") {
                        input["exclude"].push(includeText);
                    }
                }
            }
        }
    }
    return input;
}

barVisible = false;
const limit = 5;
progrssBar_increment = undefined;
progrssBar_timeout = undefined;
function showProgrssBar()
{
	const progrssBarPlaceholder = document.getElementById('progrssBarPlaceholder');

    bar = document.createElement("progress");
    bar.setAttribute("id","bar");
    bar.setAttribute("value","0");
    bar.setAttribute("max",limit);
    bar.setAttribute("style","width: 100%; height: 35px;");
    progrssBarPlaceholder.append(bar);

    cancel_button = document.createElement("button");
    cancel_button.setAttribute("type","button");
    cancel_button.setAttribute("name","cancel_button");
    cancel_button.setAttribute("id","cancel_button");
    cancel_button.setAttribute("onclick","vscode.postMessage({command: \"cancelRun\",text: \"\"});removeProgrssBar();");
    cancel_button_text = document.createTextNode("cancel");
    cancel_button.appendChild(cancel_button_text);
    progrssBarPlaceholder.append(cancel_button);

    sec = 0;
    progrssBar_increment = setInterval(() => {bar.value = (sec++)%limit;}, 1000);
}

function removeProgrssBar()
{
    clearTimeout(progrssBar_timeout);
    clearInterval(progrssBar_increment);
    document.getElementById('progrssBarPlaceholder').innerHTML="";
    barVisible = false;
}

function run()
{
    if (barVisible===true) {
        return;
    }
    cleanResults();
    input = buildInput();
    if (Object.keys(input).length > 0) {
        barVisible = true;
        showProgrssBar();
		vscode.postMessage({command: "getInput",text: input});
        // synDone = setTimeout(() => {barVisible = false;},limit*1000);
    }
    console.log(input);
}

created_buttones = false;
numOfResults = 0;
results_arr = [];

function showResult(json_str)
{
    if (!created_buttones) {
        const resultsListPlaceholder = document.getElementById('resultsListPlaceholder');

        let includes_list = document.createElement("ul");
		includes_list.setAttribute("id","includes");
		resultsListPlaceholder.append(includes_list);

        let excludes_list = document.createElement("ul");
		excludes_list.setAttribute("id","excludes");
		resultsListPlaceholder.append(excludes_list);

        //type="button" name="set_include" id="set_include" onclick="markInclude()"
		let include = document.createElement("button");
        include.setAttribute("type","button");
        include.setAttribute("name","set_include");
        include.setAttribute("id","set_include");
        include.setAttribute("onclick","markInclude()");
        include_button_text = document.createTextNode("include");
        include.appendChild(include_button_text);
		resultsListPlaceholder.append(include);

		let exclude = document.createElement("button");
        exclude.setAttribute("type","button");
        exclude.setAttribute("name","set_exclude");
        exclude.setAttribute("id","set_exclude");
        exclude.setAttribute("onclick","markExclude()");
        exclude_button_text = document.createTextNode("exclude");
        exclude.appendChild(exclude_button_text);
		resultsListPlaceholder.append(exclude);

		let results_list = document.createElement("ul");
		results_list.setAttribute("id","results");
		resultsListPlaceholder.append(results_list);

        created_buttones = true;
	}

    let json = Object.values(JSON.parse(json_str))[0];
    let regex_str = json["str"];
    let regex = json["regex"];

    let res = document.createElement("li");
    res.setAttribute("class","resultCell");
    res.setAttribute("id","result"+ numOfResults++);

	let copy_regex = document.createElement("button");
    copy_regex.setAttribute("type","button");
    copy_regex.setAttribute("name","copy_regex");
    copy_regex.setAttribute("class","copy_regex no-select");
    copy_regex.setAttribute("onclick","copyRegexToClipboard(this)");
    let copy_regex_button_text = document.createTextNode("copy to clipboard");
    copy_regex.appendChild(copy_regex_button_text);
	res.append(copy_regex);

	let show_examples = document.createElement("button");
    show_examples.setAttribute("type","button");
    show_examples.setAttribute("name","show_examples");
    show_examples.setAttribute("class","show_examples no-select");
    show_examples.setAttribute("onclick","showExamples_tx(this)");
    let show_examples_button_text = document.createTextNode("show examples");
    show_examples.appendChild(show_examples_button_text);
	res.append(show_examples);

    let res_text = document.createTextNode(regex_str);
    res.appendChild(res_text);

    let list = document.getElementById('results');
    list.append(res);

    results_arr.push(regex);
}

function cleanResults()
{
    let list = document.getElementById('results');
    if (list!==undefined && list!==null) {
        let results = list.children;
        for (let i=0; i<results.length; ++i) {
            list.removeChild(results[i--]);
        }
    }
    numOfResults = 0;
    results_arr = [];
}

addRow();

function getResultSelectionText()///////////////////////////////////////////////////
{
    let startSelectionTextNode = window.getSelection().anchorNode;
    let endSelectionTextNode = window.getSelection().focusNode;

    if (startSelectionTextNode==null
        || startSelectionTextNode!==endSelectionTextNode
        || !startSelectionTextNode.parentNode.classList.contains("resultCell")) {
        return "";
    }

    let startOffset = window.getSelection().anchorOffset;
    let endOffset = window.getSelection().focusOffset;
    let wantedStartOffset = startOffset;
    let wantedEndOffset = endOffset;

    // if (startSelectionTextNode.data[startOffset]==='>') {
    //     if (startOffset===startSelectionTextNode.data.length-1) { // last in text
    //         // <<> <?>
    //         //   ^   ^
    //         //go left until opening <
    //     } else { // there is a char after it
    //         if (startSelectionTextNode.data[startOffset+1]==='>') {
    //             // <>>
    //             //  ^
    //             wantedEndOffset = startOffset+2;// window.getSelection().modify("extend", "forward", "character");
    //         } else {
    //             // <<> <?>
    //             //   ^   ^
    //             //go left until opening <
    //         }
    //     }
    // } else if (startSelectionTextNode.data[startOffset]==='(') {
    //     if (startOffset===startSelectionTextNode.data.length-1) { // last in text
    //         // <<> <?>
    //         //   ^   ^
    //         //go left until opening <
    //     } else { // there is a char after it
    //         if (startSelectionTextNode.data[startOffset+1]==='>') {
    //             // <>>
    //             //  ^
    //             wantedEndOffset = startOffset+2;
    //         } else {
    //             // <<> <?>
    //             //   ^   ^
    //             //go left until opening <
    //         }
    //     }
    // } else {
    //     // go right until closing >, then left until opening <
    //     //or go right until opening (, then left until
    //     let found_rparen = false;
    //     let found_rarrow = false;
    //     for (let i=startOffset; startSelectionTextNode.data.length; ++i) {
            
    //     }
    // }

    let range = new Range();
    range.setStart(startSelectionTextNode, startOffset);
    range.setEnd(startSelectionTextNode, endOffset);

    window.getSelection().extend(startSelectionTextNode,wantedEndOffset);
    var text = window.getSelection().toString();

    // console.log("text: "+text);
    text_stripped = "";
    let opened_arrow = false;
    let read_char = false;
    // <<> <>> <?>
    for (let c = 0; c < text.length; c++) {
        if (text[c]=="<") {
            if (opened_arrow) {
                // <<>
                // <^
                text_stripped+=text[c];
                read_char = true;
            } else {
                // <<>
                // <>>
                // <?>
                // ^
                opened_arrow = true;
            }
        } else if (text[c]==">") {
            if (!read_char) {
                // <>>
                // <^
                text_stripped+=text[c];
                read_char = true;
            } else {
                // <<>
                // <>>
                // <?>
                // <*^
                opened_arrow = false;
                read_char = false;
            }
        } else {
            // <?>
            // ???
            // *^
            if (opened_arrow) {
                // <?>
                // <^
                text_stripped+=text[c];
                read_char = true;
            } else {
                // ???
                text_stripped+=text[c];
            }
        }
    }
    // console.log("text without </>: "+text_stripped);
    return text_stripped;
}

//check if valid: word with no letter before or after | <?> | word(?,?)
//maybe add buttons wall for blocks to exclude/include. also have text [ or(<num>,<let>) ]

function markInclude()
{
    toInclude = getResultSelectionText();
    if (toInclude!=="") {
        colorize("ForeColor", false, "limegreen");

        let inc = document.createElement("li");
        inc.setAttribute("class","includeCell");

        let remove_button = document.createElement("button");
        remove_button.setAttribute("type", "button");
        remove_button.innerHTML = "Remove";
        remove_button.addEventListener("click", function () {
            this.parentNode.parentNode.removeChild(this.parentNode);
        });
        inc.appendChild(remove_button);

        let inc_text = document.createTextNode(toInclude);
        inc.appendChild(inc_text);

        let include_list = document.getElementById('includes');
        include_list.append(inc);
    }
}

function markExclude()
{
    toExclude = getResultSelectionText();
    if (toExclude!=="") {
        colorize("ForeColor", false, "tomato");

        let exc = document.createElement("li");
        exc.setAttribute("class","excludeCell");

        let remove_button = document.createElement("button");
        remove_button.setAttribute("type", "button");
        remove_button.innerHTML = "Remove";
        remove_button.addEventListener("click", function () {
            this.parentNode.parentNode.removeChild(this.parentNode);
        });
        exc.appendChild(remove_button);

        let exc_text = document.createTextNode(toExclude);
        exc.appendChild(exc_text);

        let exclude_list = document.getElementById('excludes');
        exclude_list.append(exc);
    }
}

function generateTable(json) {
    var table = document.createElement("table");
    var thead = document.createElement("thead");
    table.appendChild(thead);
    var tbody = document.createElement("tbody");
    table.appendChild(tbody);
    // Iterate over each cluster
    for (var key in json) {
        var cluster = json[key];
        var examples = cluster.examples;
        var rowCount = Object.values(examples).length;

        // Create a row for the explanation
        var trExpl = document.createElement("tr");
        var thExpl = document.createElement("th");
        thExpl.innerHTML = "";
        let exp = document.createTextNode(cluster.explanation);
        thExpl.appendChild(exp);
        thExpl.setAttribute("colspan", "3");
        trExpl.appendChild(thExpl);
        tbody.appendChild(trExpl);

        // Create a variable to keep track of the current example index
        var curExampleIndex = -1;

        // Iterate over each example in the cluster
        for (var i = 0; i < rowCount; i++) {
            var example = examples[i];

            if (example.Result === false && example.index !== undefined) {
                curExampleIndex = example.index;
                var exampleStr = example.example.toString();
                var exampleArr = exampleStr.split("");
                exampleArr[curExampleIndex] = '<span style="color: red;">' + exampleArr[curExampleIndex] + "</span>";
                example.example = exampleArr.join("");
            }

            // Create a row for the example
            var trExample = document.createElement("tr");
            var tdExample = document.createElement("td");
            var tdResult = document.createElement("td");
            var tdEdit = document.createElement("td");

            tdExample.innerHTML = example.example;
            tdResult.innerHTML = example.Result ? "acc" : "rej";
            tdResult.style.color = example.Result ? "green" : "red";

            // Add an edit button to the row
            var editButton = document.createElement("button");
            editButton.innerHTML = "add to examples table";
            editButton.onclick = function () {
                var exampleRow = this.parentElement.parentElement;

                // Create a new row for the other table
                while(document.getElementById("examples")===null);
                var otherTableRow = document.getElementById("examples").lastChild;
                otherTableRow.innerHTML = row;
                otherTableRow.children[1].innerHTML = exampleRow.cells[0].textContent;

                let accSel = "";
                let rejSel = "";
                if (exampleRow.cells[1].innerHTML==="acc") {
                    accSel = ' selected="selected"';
                } else {
                    rejSel = ' selected="selected"';
                }

                otherTableRow.children[2].children[0].innerHTML = ' \
                    <option value="acc"'+accSel+'>Accept</option>   \
                    <option value="rej"'+rejSel+'>Reject</option>';

                addRow();
                exampleRow.remove();
            };

            tdEdit.appendChild(editButton);

            trExample.appendChild(tdExample);
            trExample.appendChild(tdResult);
            trExample.appendChild(tdEdit);
            tbody.appendChild(trExample);
        }
    }
    return table;
}

function copyRegexToClipboard(buttonElem) {
    var resultItem = buttonElem.parentElement;
    var index = Number(resultItem.id.substr(6));
    console.log("copy: results_arr["+index+"]="+results_arr[index]);
    vscode.postMessage({command: "copyToClipboard", regex: results_arr[index]});
}

function showExamples_tx(buttonElem) {
    document.getElementById("examplesTablePlaceholder").innerHTML = "";

    var example = "";
    var rows = document.getElementById("examples").childNodes;
    for (let i=0; i<rows.length-1; ++i) {
        current_row = rows[i].children;
        if (current_row[2].children[0].value=="acc") {
            example = current_row[1].innerText;
            break;
        }
    }

    var resultItem = buttonElem.parentElement;
    var index = Number(resultItem.id.substr(6));

    document.getElementById("examplesTablePlaceholder").innerHTML = "";
    let regex_str = document.createTextNode(resultItem.lastChild.wholeText);
    document.getElementById("examplesTablePlaceholder").appendChild(regex_str);
    vscode.postMessage({command: "getExamples", example: example, regex: results_arr[index]});
}

function showExamples_rx(json) {
    var table = generateTable(json);
    document.getElementById("examplesTablePlaceholder").appendChild(table);
}
