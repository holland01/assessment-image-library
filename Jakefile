var child_proc = require('child_process');

function handler(error, stdout, stderr) {
	jake.logger.log('Finished: ' + "rebuild");
      	
	process.stdout.write(stdout);
    
	if (error !== null) {
      	fail(error, error.code);
	}
}

function run(command, onfinish) {
	var child = child_proc.exec(command, handler);
	child.stdout.pipe(process.stdout);
	child.stderr.pipe(process.stderr);
	
	if (onfinish) {
		child.on("exit", function() {
			if (Array.isArray(onfinish)) {
				run(onfinish[0], onfinish[1]);
			} else if (typeof(onfinish) === "string") {
				run(onfinish, null);
			} else {
				throw "bad type; string or arrays are supported already";
			}
		});
	}
}

task("build", [], function (params) {
	run("make clean", ["make", "emrun testbin.html"]);
});
