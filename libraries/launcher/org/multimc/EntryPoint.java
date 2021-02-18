package org.multimc;/*
 * Copyright 2012-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import org.multimc.onesix.OneSixLauncher;

import java.io.*;
import java.nio.charset.Charset;

public class EntryPoint
{
    private enum Action
    {
        Proceed,
        Launch,
        Abort
    }

    public static void main(String[] args)
    {
        EntryPoint listener = new EntryPoint();
        int retCode = listener.listen();
        if (retCode != 0)
        {
            System.out.println("Exiting with " + retCode);
            System.exit(retCode);
        }
    }

    private Action parseLine(String inData) throws ParseException
    {
        String[] pair = inData.split(" ", 2);

        if(pair.length == 1)
        {
            String command = pair[0];
            if (pair[0].equals("launch"))
                return Action.Launch;

            else if (pair[0].equals("abort"))
                return Action.Abort;

            else throw new ParseException("Error while parsing:" + pair[0]);
        }

        if(pair.length != 2)
            throw new ParseException("Pair length is not 2.");

        String command = pair[0];
        String param = pair[1];

        if(command.equals("launcher"))
        {
            if(param.equals("onesix"))
            {
                m_launcher = new OneSixLauncher();
                Utils.log("Using onesix launcher.");
                Utils.log();
                return Action.Proceed;
            }
            else
                throw new ParseException("Invalid launcher type: " + param);
        }

        m_params.add(command, param);
        //System.out.println(command + " : " + param);
        return Action.Proceed;
    }

    public int listen()
    {
        BufferedReader buffer;
        try
        {
            buffer = new BufferedReader(new InputStreamReader(System.in, "UTF-8"));
        } catch (UnsupportedEncodingException e)
        {
            System.err.println("For some reason, your java does not support UTF-8. Consider living in the current century.");
            e.printStackTrace();
            return 1;
        }
        boolean isListening = true;
        boolean isAborted = false;
        // Main loop
        while (isListening)
        {
            String inData;
            try
            {
                // Read from the pipe one line at a time
                inData = buffer.readLine();
                if (inData != null)
                {
                    Action a = parseLine(inData);
                    if(a == Action.Abort)
                    {
                        isListening = false;
                        isAborted = true;
                    }
                    if(a == Action.Launch)
                    {
                        isListening = false;
                    }
                }
                else
                {
                    isListening = false;
                    isAborted = true;
                }
            }
            catch (IOException e)
            {
                System.err.println("Launcher ABORT due to IO exception:");
                e.printStackTrace();
                return 1;
            }
            catch (ParseException e)
            {
                System.err.println("Launcher ABORT due to PARSE exception:");
                e.printStackTrace();
                return 1;
            }
        }
        if(isAborted)
        {
            System.err.println("Launch aborted by MultiMC.");
            return 1;
        }
        if(m_launcher != null)
        {
            return m_launcher.launch(m_params);
        }
        System.err.println("No valid launcher implementation specified.");
        return 1;
    }

    private ParamBucket m_params = new ParamBucket();
    private org.multimc.Launcher m_launcher;
}
