module Main where

import System.Exit
import System.Environment
import System.IO
import System.IO.Error
import Control.Concurrent.Thread.Delay

waitingGetChar =
    catchIOError getChar (\e -> if isEOFError e then return ' ' else ioError e)

waitForWriter pipe_path = do
    h <- openFile pipe_path ReadMode
    eof <- hIsEOF h
    if eof
        then do
            delay (100 * 1000)
            waitForWriter pipe_path
        else return h

loopEcho h = do
    mbCh <- tryIOError (hGetChar h) 
    case mbCh of
        Left e -> if isEOFError e
            then return ()
            else ioError e
        Right ch -> do
            putStrLn [ch]
            loopEcho h

main = do
    args <- getArgs
    case args of
        [pipe_path] -> do
            h <- waitForWriter pipe_path
            loopEcho h
        _ -> do
            pgName <- getProgName
            hPutStrLn stderr "Invalid arguments. Should be:"
            hPutStrLn stderr $ pgName ++ " <path_pipe>"
            exitFailure
