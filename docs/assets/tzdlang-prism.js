// Prism.js language definition for TzdLang (TZD)
(function (Prism) {
    if (typeof Prism === 'undefined') return;

    Prism.languages.tzdlang = {
        'comment': [
            {
                pattern: /(^|[^\\])\/\*[\s\S]*?(?:\*\/|$)/,
                lookbehind: true,
                greedy: true
            },
            {
                pattern: /(^|[^\\:])\/\/.*/,
                lookbehind: true,
                greedy: true
            }
        ],
        'string': {
            pattern: /(["'])(?:\\(?:\r\n|[\s\S])|(?!\1)[^\\\r\n])*\1/,
            greedy: true
        },
        'annotation': {
            pattern: /@\w+(?:\([^)]*\))?/,
            alias: 'decorator',
            greedy: true
        },
        'class-name': [
            {
                pattern: /(\b(?:class|extends|new|in)\s+)[a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)*/,
                lookbehind: true
            },
            {
                pattern: /\b[A-Z][a-zA-Z0-9_]*\b/
            }
        ],
        'keyword': /\b(?:var|let|const|fun|fn|function|ret|return|class|extends|super|new|this|if|else|while|for|break|continue|switch|case|default|try|catch|throw|native|import|in|enum|static|abstract|public|private|protected|true|false|null)\b/,
        'builtin-type': {
            pattern: /\b(?:int|float|double|string|bool|void|ptr|pointer|hwnd)\b/,
            alias: 'type'
        },
        'builtin': /\b(?:print|printf|out|clock|toString|len|range|sleep|jsonParse|mapKeys|torch_[a-zA-Z0-9_]+)\b/,
        'number': /\b(?:0[xX][0-9a-fA-F]+|\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\b/,
        'operator': /--|\+\+|&&|\|\||<=|>=|==|!=|\+=|-=|\*=|(?:\/=|%=|\^=)|[-+*\/%^=<>!]/,
        'punctuation': /[{}[\];(),.:]/
    };

    Prism.languages.tzd = Prism.languages.tzdlang;
}(typeof Prism !== 'undefined' ? Prism : null));
