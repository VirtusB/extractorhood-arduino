var SpeechRecognition = SpeechRecognition || webkitSpeechRecognition;
var SpeechGrammarList = SpeechGrammarList || webkitSpeechGrammarList;
var SpeechRecognitionEvent = SpeechRecognitionEvent || webkitSpeechRecognitionEvent;

var commands = ['turn on extractor hood', 'turn on light', 'exhaust speed slow', 'exhaust speed fast', 'turn off extractor hood', 'turn off light'];

var grammar = '#JSGF V1.0; grammar extractorHoodCommands; public <command> = ' + commands.join(' | ') + ' ;';

var recognition = new SpeechRecognition();
var speechRecognitionList = new SpeechGrammarList();
speechRecognitionList.addFromString(grammar, 1);

recognition.grammars = speechRecognitionList;
recognition.continuous = false;
recognition.lang = 'en-US';
recognition.interimResults = false;
recognition.maxAlternatives = 1;

var diagnostic = document.querySelector('.output');

document.getElementById('voice-btn').addEventListener('click', function (event) {
    recognition.start();
    console.log('Ready to receive a command.');
});

recognition.onresult = function(event) {
    var command = event.results[0][0].transcript;
    console.log('Command: ' + command);
    diagnostic.textContent = 'Result received: ' + command + '.';
    console.log('Confidence: ' + event.results[0][0].confidence);

    command = command.toLowerCase();

    handleSpeechCommand(command);
};

function handleSpeechCommand(command) {
    switch (command) {
        case 'turn on extractor hood':
            console.log('tænd for emhætten');
            if (confirm('Tænd for emhætten?')) {
                location.pathname = '/5/on';
            }
            break;
        case 'turn on light':
            console.log('tænd for lyset på emhætten');
            if (confirm('Tænd for lyset på emhætten?')) {
                location.pathname = '/4/on';
            }
            break;
        case 'exhaust speed slow':
            console.log('sæt blæser hastighed til langsom');
            if (confirm('Sæt blæser hastighed til langsom?')) {
                location.pathname = '/6/slow';
            }
            break;
        case 'exhaust speed fast':
            console.log('sæt blæser hastighed til hurtig');
            if (confirm('Sæt blæser hastighed til hurtig?')) {
                location.pathname = '/6/fast';
            }
            break;
        case 'turn off extractor hood':
            console.log('sluk for emhætten');
            if (confirm('Sluk for emhætten?')) {
                location.pathname = '/5/off';
            }
            break;
        case 'turn off light':
            console.log('sluk for lyset');
            if (confirm('Sluk for lyset?')) {
                location.pathname = '/4/off';
            }
            break;
        default:
            let found = tryToFindCommandBetter(command);

            if (found !== false) {
                console.log('Fandt kommando ved at søge commands igennem');
                console.log('Kommando fundet:' + found);
                handleSpeechCommand(found);
            } else {
                console.log('Kunne ikke finde kommando')
            }
            break;
    }
}

function tryToFindCommandBetter(speechCommand) {
    let found = false;

    commands.forEach(cmd => {
        let splitSpeechCommand = speechCommand.split(' ');
        let splitCommand = cmd.split(' ');

        let containsEverySubString = splitCommand.every(val => {
            console.log('val: ' + val);
            console.log('indexOf: ' + splitCommand.indexOf(val));
            console.log(splitCommand);
            return splitSpeechCommand.indexOf(val) >= 0;
        });
        console.log(containsEverySubString);
        if (containsEverySubString) {
            found = cmd;
        }
    });

    return found;
}

recognition.onspeechend = function() {
    recognition.stop();
};

recognition.onnomatch = function(event) {
    diagnostic.textContent = 'Ingen kommando fundet, prøv igen';
};

recognition.onerror = function(event) {
    diagnostic.textContent = 'Fejl i stemmestyring: ' + event.error;
};