
(module embed-sig mzscheme
  (require (lib "unitsig.ss"))
  (provide compiler:embed^)

  (define-signature compiler:embed^
    (create-embedding-executable
     make-embedding-executable
     write-module-bundle
     embedding-executable-is-directory?
     embedding-executable-is-actually-directory?
     embedding-executable-put-file-extension+style+filters
     embedding-executable-add-suffix)))