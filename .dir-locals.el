;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((c++-mode
  (flycheck-clang-args `("-lllvm" ,(shell-command-to-string "llvm-config --cxxflags")))
  (canary . "tweet, tweet!")
  (flycheck-clang-language-standard . "c++14")))

(setq flycheck-clang-args `(,@(split-string (shell-command-to-string "llvm-config --cxxflags")) "--std=c++14"))
