import * as vscode from 'vscode';
import {readFileSync, existsSync} from "fs";
import {spawn, spawnSync, ChildProcessWithoutNullStreams} from "child_process";
import {Socket, connect} from "net";

let regex_currentPanel: vscode.WebviewPanel | undefined = undefined;
let server_exe: ChildProcessWithoutNullStreams|undefined = undefined;
let client: Socket | undefined = undefined;

export function activate(context: vscode.ExtensionContext) {
	console.log('console.log: Congratulations, your extension "extension-ts (regex synthesizing)" is now active!');

	context.subscriptions.push(vscode.commands.registerCommand('extension-ts.startRegexSyn', () => {
		if (regex_currentPanel!==undefined) {
			const columnToShowIn = vscode.window.activeTextEditor ? vscode.window.activeTextEditor.viewColumn : undefined;
			regex_currentPanel.reveal(columnToShowIn);
			return;
		}

		regex_currentPanel = vscode.window.createWebviewPanel(
			'regexSyn',
			'Regex Synthesizer',
			vscode.ViewColumn.Beside,
			{
				enableScripts: true,
				retainContextWhenHidden: true,
				localResourceRoots: [vscode.Uri.joinPath(context.extensionUri, 'src')]
			}
		);
		regex_currentPanel.webview.html = "<h1>LOADING...</h1>";
		regex_currentPanel.onDidDispose(() => {
			regex_currentPanel = undefined;
			if (server_exe!==undefined) {
				const kill_server_exe = spawnSync("taskkill", ["/PID",String(server_exe.pid),"/F","/T"]);
				console.log(`kill_server_exe.status=${kill_server_exe.status}`);
				if (kill_server_exe.status!==0) {
					console.log("Faild to kill onDidDispose server.exe!");
				}
				server_exe = undefined;
			}
			if (client!==undefined) {
				console.log("onDidDispose client.destroy()");
				client.destroy();
				client = undefined;
			}
		}, null, context.subscriptions);

		if (!existsSync(context.extensionPath+"\\server.exe")) {
			const compile_bat = spawnSync('cmd.exe', ['/c', context.extensionPath+`\\compile.bat ${context.extensionPath}`], {encoding: 'utf-8'});
			console.log(`compile_bat.status=${compile_bat.status}`);
			if (compile_bat.status!==0) {
				console.log("Faild to build server.exe!");
				console.log(compile_bat.stderr);
				vscode.window.showErrorMessage("Faild to build server.exe!");
				regex_currentPanel.webview.html = "<h1>ERROR!!!</h1>";
				return;
			}
		}
		if (!existsSync(context.extensionPath+"\\ExplanationGenerator.class")
			|| !existsSync(context.extensionPath+"\\InputGenerator.class")) {
			const javac = spawnSync('cmd.exe', ['/c',"\"javac -cp \""+context.extensionPath+";"+context.extensionPath+"\\java_folder\\lib\\automaton.jar;"+context.extensionPath+"\\java_folder\\lib\\json.jar\" \""+context.extensionPath+"\\InputGenerator.java\" \""+context.extensionPath+"\\ExplanationGenerator.java\"\""], {encoding: 'utf-8', windowsVerbatimArguments: true});
			console.log(`javac.status=${javac.status}`);
			if (javac.status!==0) {
				console.log("Faild to build java files!");
				console.log(javac.stderr);
				vscode.window.showErrorMessage("Faild to build java files!");
				regex_currentPanel.webview.html = "<h1>ERROR!!!</h1>";
				return;
			}
		}

		server_exe = spawn('cmd.exe', ['/c', context.extensionPath+"\\server.exe"]);
		let error: boolean = false;
		server_exe.on("error", ()=>{
			error = true;
			console.log("Faild to run server.exe!");
			vscode.window.showErrorMessage("Faild to run server.exe!");
			if (regex_currentPanel) {regex_currentPanel.webview.html = "<h1>ERROR!!!</h1>";}
		});
		if (error) {return;}
		server_exe.on("spawn", ()=>{console.log("server.exe is running...");});
		server_exe.stdout.on('data', (data)=>{console.log(`server_exe stdout: ${data}`);});
		server_exe.on('close', (code)=>{console.log(`server_exe closed all stdio with code ${code}`);});
		server_exe.on('exit', (code)=>{console.log(`server_exe exited with code ${code}`);});
		server_exe.on('disconnect', ()=>{console.log(`server_exe disconnected with code`);});
		server_exe.on('message', (code)=>{console.log(`server_exe messaged with code ${code}`);});

		regex_currentPanel.webview.html = readFileSync(context.extensionPath+"\\src\\index.html", "utf8");

		regex_currentPanel.webview.onDidReceiveMessage(async (message) => {
			if (regex_currentPanel===undefined) {
				return;
			}

			switch (message.command) {
				case "getCSSnJS":
					const stylePath = vscode.Uri.joinPath(context.extensionUri, "src", "style.css");
					const styleUri = regex_currentPanel.webview.asWebviewUri(stylePath);
					const scriptPath = vscode.Uri.joinPath(context.extensionUri, "src", "script.js");
					const scriptUri = regex_currentPanel.webview.asWebviewUri(scriptPath);
					regex_currentPanel.webview.postMessage({command: "setCSSnJS", text: {styleUri: String(styleUri), scriptUri: String(scriptUri), cspSource: String(regex_currentPanel.webview.cspSource)}});
					break;
				case "getGeneral":
					let input = await vscode.window.showQuickPick(
						[
							{label: 'any', description: 'any character', picked: true},
							{label: 'num', description: 'numbers from 0 to 9'},
							{label: 'num1-9', description: 'numbers from 1 to 9'},
							{label: 'let', description: 'all the english letters'},
							{label: 'low', description: 'all the small english letters'},
							{label: 'cap', description: 'all the capital english letters'},
						]
						,{title:"choose the generalization of selected characters",canPickMany: false,ignoreFocusOut:true}
					);
					if (input===undefined) {
						regex_currentPanel.webview.postMessage({command: "cancelGeneral"});
						break;
					}
					regex_currentPanel.webview.postMessage({command: "setGeneral", text: input.label});
					break;
				case "getInput":
					if (client!==undefined) {
						console.log("getInput client.destroy()");
						client.destroy();
						client = undefined;
					}
					client = connect(27015,"localhost",() => {console.log("Connected to server");});
					client.write(JSON.stringify(message.text));
					client.on("data", (data) => {
						console.log("client.on(data): "+data.toString("utf-8"));
						let regexes = splitJsons(data.toString("utf-8"));
						console.log("splitJsons(data.toString(\"utf-8\"): ");
						console.log(regexes);
						for (let reg of regexes) {
							console.log("reg: "+reg);
							regex_currentPanel?.webview.postMessage({command: "showResult", text: reg});
							if ('4' in JSON.parse(reg)) {
								console.log("reg is last");
								regex_currentPanel?.webview.postMessage({command: "doneResult", text: ""});
								if (client!==undefined) {
									console.log("client.on(data) client.destroy()");
									client.destroy();
									client = undefined;
								}
							}
						}
					});
					client.on("end", () => {console.log("client.on(end)");});
					client.on("close", () => {console.log("client.on(close)");});
					client.on("timeout", () => {console.log("client.on(timeout)");});
					break;
				case "cancelRun":
					server_exe = cancelRun(regex_currentPanel,context.extensionPath,server_exe,client);
					break;
				case "getExamples":
					const java_examples = spawnSync('cmd.exe', ['/c',"\"java -cp \""+context.extensionPath+";"+context.extensionPath+"\\java_folder\\lib\\automaton.jar;"+context.extensionPath+"\\java_folder\\lib\\json.jar\" InputGenerator \""+message.example+"\" \""+message.regex+"\"\""], {encoding: 'utf-8', windowsVerbatimArguments: true});
					console.log(`java_examples.status=${java_examples.status}`);
					if (java_examples.status!==0) {
						console.error("Faild to generate examples!");
						console.error(java_examples.stderr);
						vscode.window.showErrorMessage("Faild to generate examples!");
						return;
					}
					regex_currentPanel.webview.postMessage({command: "showExamples", text: java_examples.stdout});
				break;
				case "copyToClipboard":
					console.log("clipboard: "+message.regex)
					vscode.env.clipboard.writeText(message.regex);
					vscode.window.showInformationMessage("copied to clipboard");
					break;
				default:
					break;
			}
		},undefined,context.subscriptions);

	}));
}

export function deactivate() {
	if (server_exe!==undefined) {
		const kill_server_exe = spawnSync("taskkill", ["/PID",String(server_exe.pid),"/F","/T"]);
		console.log(`deactivate kill_server_exe.status=${kill_server_exe.status}`);
		if (kill_server_exe.status!==0) {
			console.error("Faild to kill deactivate server.exe!");
			console.error(kill_server_exe.stderr);
			vscode.window.showErrorMessage("Faild to kill deactivate server.exe!");
		}
		server_exe = undefined;
	}

	client?.destroy();
	client = undefined;

	// regex_currentPanel?.dispose();
	// regex_currentPanel = undefined;
}

function cancelRun(regex_currentPanel: vscode.WebviewPanel, extensionPath: string, server_exe: ChildProcessWithoutNullStreams|undefined , client: Socket|undefined) {
	if (client!==undefined) {
		console.log("cancelRun client.destroy()");
		client.destroy();
		client = undefined;
	}
	if (server_exe===undefined) {
		return server_exe;
	}

	const kill_server_exe = spawnSync('taskkill', ["/PID",String(server_exe.pid),"/F","/T"]);
	console.log(`kill_server_exe.status=${kill_server_exe.status}`);
	if (kill_server_exe.status!==0) {
		console.error("Faild to kill old server.exe!");
		vscode.window.showErrorMessage("Faild to kill old server.exe!");
		regex_currentPanel.webview.html = "<h1>ERROR!!!</h1>";
		return server_exe;
	}

	server_exe = spawn('cmd.exe', ['/c', extensionPath+"\\server.exe"]);
	server_exe.on("error", ()=>{
		console.error("Faild to run server.exe!");
		vscode.window.showErrorMessage("Faild to run server.exe!");
		if (regex_currentPanel) {regex_currentPanel.webview.html = "<h1>ERROR!!!</h1>";}
	});
	server_exe.on("spawn", ()=>{console.log("server.exe is running...");});
	server_exe.stdout.on('data', (data)=>{console.log(`server_exe stdout: ${data}`);});
	server_exe.on('close', (code)=>{console.log(`server_exe closed all stdio with code ${code}`);});
	server_exe.on('exit', (code)=>{console.log(`server_exe exited with code ${code}`);});
	server_exe.on('disconnect', ()=>{console.log(`server_exe disconnected with code`);});
	server_exe.on('message', (code)=>{console.log(`server_exe messaged with code ${code}`);});
	return server_exe;
}

function splitJsons(jsons: string) : string[] {
	if (jsons.indexOf('\n')===jsons.length-1) {
		return [jsons.slice(0,-1)];
	}
	let arr = [jsons.substring(0,jsons.indexOf('\n'))];
	return arr.concat(splitJsons(jsons.substring(jsons.indexOf('\n')+1)));
}
